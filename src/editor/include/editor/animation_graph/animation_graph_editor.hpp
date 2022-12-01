#pragma once

#include <map>
#include <vector>

#include <imgui.h>
#include <imnodes.h>

#include <anim/graph/runtime_graph_instance.hpp>

#include "editor_graph_node.hpp"
#include "link.hpp"

namespace aln
{

/// @brief A stateful representation of an animation graph, which contains all of its data.
/// During serialization its node are translated into the runtime format, and their data stored in a runtime graph definition
/// so that multiple instances of the same graph can share it.
class AnimationGraphEditor
{
  private:
    std::vector<EditorGraphNode*> m_graphNodes; // TODO: Replace with actual graph
    std::vector<Link> m_links;

    std::map<UUID, EditorGraphNode*> m_nodeLookupMap;
    std::map<UUID, const Pin*> m_pinLookupMap;

    UUID m_contextPopupElementID;

  public:
    void Draw()
    {
        UUID hoveredNodeID, hoveredPinID, hoveredLinkID;
        bool nodeHovered = ImNodes::IsNodeHovered(&hoveredNodeID);
        bool pinHovered = ImNodes::IsPinHovered(&hoveredPinID);
        bool linkHovered = ImNodes::IsLinkHovered(&hoveredLinkID);

        ImNodes::BeginNodeEditor();

        // Open contextual popups and register context ID
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImNodes::IsEditorHovered())
        {
            if (pinHovered)
            {
                m_contextPopupElementID = hoveredPinID;
                ImGui::OpenPopup("graph_editor_pin_popup");
            }
            else if (nodeHovered)
            {
                m_contextPopupElementID = hoveredNodeID;
                ImGui::OpenPopup("graph_editor_node_popup");
            }
            else if (linkHovered)
            {
                m_contextPopupElementID = hoveredLinkID;
                ImGui::OpenPopup("graph_editor_link_popup");
            }
            else
            {
                m_contextPopupElementID = UUID::InvalidID();
                ImGui::OpenPopup("graph_editor_canvas_popup");
            }
        }

        if (ImGui::BeginPopup("graph_editor_canvas_popup"))
        {
            const ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();

            if (ImGui::BeginMenu("Add Node"))
            {
                auto& animGraphNodeTypes = aln::reflect::GetTypesInScope("ANIM_GRAPH_NODE");
                for (auto& pAnimGraphNodeType : animGraphNodeTypes)
                {
                    if (ImGui::Selectable(pAnimGraphNodeType->GetPrettyName().c_str()))
                    {
                        auto pNode = pAnimGraphNodeType->typeHelper->CreateType<EditorGraphNode>();
                        pNode->Initialize();

                        AddGraphNode(pNode);

                        ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_pin_popup"))
        {
            ImGui::MenuItem("Pin !");
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_node_popup"))
        {
            if (ImGui::MenuItem("Remove node"))
            {
                RemoveGraphNode(m_contextPopupElementID);
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_link_popup"))
        {
            if (ImGui::MenuItem("Remove link"))
            {
                RemoveLink(m_contextPopupElementID);
            }
            ImGui::EndPopup();
        }

        // Draw nodes
        for (auto pNode : m_graphNodes)
        {
            ImNodes::BeginNode(pNode->GetID());

            ImNodes::BeginNodeTitleBar();
            ImGui::Text(pNode->GetName().c_str());
            ImNodes::EndNodeTitleBar();

            for (auto& inputPin : pNode->m_inputPins)
            {
                ImNodes::BeginInputAttribute(inputPin.GetID());
                ImGui::Text(inputPin.GetName().c_str());
                ImNodes::EndInputAttribute();
            }

            // TODO: Display reflected fields

            for (auto& outputPin : pNode->m_outputPins)
            {
                ImNodes::BeginOutputAttribute(outputPin.GetID());
                ImGui::Text(outputPin.GetName().c_str());
                ImNodes::EndOutputAttribute();
            }

            ImNodes::EndNode();
        }

        // Draw links
        for (auto& link : m_links)
        {
            ImNodes::Link(link.m_id, link.m_inputPinID, link.m_outputPinID);
        }

        ImNodes::EndNodeEditor();

        // Handle link creation
        UUID startPinID, endPinID, startNodeID, endNodeID;
        if (ImNodes::IsLinkCreated(&startNodeID, &startPinID, &endNodeID, &endPinID))
        {
            AddLink(startNodeID, startPinID, endNodeID, endPinID);
        }

        UUID linkID;
        if (ImNodes::IsLinkDestroyed(&linkID))
        {
            RemoveLink(linkID);
        }
    }

    EditorGraphNode* FindNode(UUID& nodeID)
    {
        assert(nodeID.IsValid());
        // TODO : Use actual graph
        return m_nodeLookupMap[nodeID];
    }

    /// @brief Create a graph node of a selected type
    void AddGraphNode(EditorGraphNode* pNode)
    {
        assert(pNode != nullptr);

        // TODO: Add the node to the actual graph
        m_graphNodes.push_back(pNode);

        // Populate lookup maps
        m_nodeLookupMap[pNode->GetID()] = pNode;
        for (auto& pin : pNode->m_inputPins)
        {
            m_pinLookupMap[pin.GetID()] = &pin;
        }
        for (auto& pin : pNode->m_outputPins)
        {
            m_pinLookupMap[pin.GetID()] = &pin;
        }
    }

    /// @brief Remove a node from the graph
    void RemoveGraphNode(const UUID& nodeID)
    {
        assert(nodeID.IsValid());

        auto pNode = m_nodeLookupMap.at(nodeID);

        // Clean up lookup maps
        m_nodeLookupMap.erase(pNode->GetID());
        for (auto& pin : pNode->m_inputPins)
        {
            m_pinLookupMap.erase(pin.GetID());
        }
        for (auto& pin : pNode->m_outputPins)
        {
            m_pinLookupMap.erase(pin.GetID());
        }

        // Remove the node's attached links
        std::erase_if(m_links, [&](auto& link)
            { return link.m_pInputNode == pNode || link.m_pOutputNode == pNode; });

        // TODO: Actually remove the node from the graph
        std::erase(m_graphNodes, pNode);

        aln::Delete(pNode);
    }

    /// @brief Create a link between two pins
    void AddLink(UUID& inputNodeID, UUID& inputPinID, UUID& outputNodeID, UUID& outputPinID)
    {
        assert(inputNodeID.IsValid() && inputPinID.IsValid() && outputNodeID.IsValid() && outputPinID.IsValid());

        auto pInputPin = m_pinLookupMap[inputPinID];
        auto pOutputPin = m_pinLookupMap[outputPinID];

        // Ensure matching pin types
        if (pInputPin->GetValueType() != pOutputPin->GetValueType())
        {
            return;
        }

        // Ensure pins are input/output and in the right order
        if (!pInputPin->IsInput())
        {
            std::swap(pInputPin, pOutputPin);
            std::swap(inputNodeID, outputNodeID);
        }

        assert(pInputPin->IsInput() && pOutputPin->IsOutput());

        auto& link = m_links.emplace_back();
        link.m_pInputNode = m_nodeLookupMap[inputNodeID];
        link.m_inputPinID = pInputPin->GetID();
        link.m_pOutputNode = m_nodeLookupMap[outputNodeID];
        link.m_outputPinID = pOutputPin->GetID();
    }

    void RemoveLink(UUID& linkID)
    {
        assert(linkID.IsValid());
        std::erase_if(m_links, [&](Link& link)
            { return link.m_id == linkID; });
    }

    /// @brief
    void Serialize()
    {
        // TODO
    }
};
} // namespace aln