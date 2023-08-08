#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "graph/editor_graph_node.hpp"
#include "graph/editor_graph.hpp"

namespace aln
{

/// @brief Stateful view of an editor graph
class GraphView
{
  private:
    EditorGraph* m_pGraph = nullptr;
    UUID m_contextPopupElementID = UUID::InvalidID;

  public:
    void SetViewedGraph(EditorGraph* pGraph) { m_pGraph = pGraph; }

    void Clear()
    {
        m_pGraph = nullptr;
        m_contextPopupElementID = UUID::InvalidID;
    }

    // TODO: Move registry in the context ?
    // TODO: Should context be shared between view instances ?
    void Draw(const TypeRegistryService* pTypeRegistryService, GraphDrawingContext& drawingContext)
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
                m_contextPopupElementID = UUID::InvalidID;
                ImGui::OpenPopup("graph_editor_canvas_popup");
            }
        }

        if (ImGui::BeginPopup("graph_editor_canvas_popup"))
        {
            const ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
            if (ImGui::BeginMenu("Add Node"))
            {
                auto pSelectedNodeType = m_pGraph->AvailableNodeTypesMenuItems(pTypeRegistryService);
                if (pSelectedNodeType != nullptr)
                {
                    auto pNode = pSelectedNodeType->CreateTypeInstance<EditorGraphNode>();
                    pNode->Initialize();

                    m_pGraph->AddGraphNode(pNode);

                    ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
                }
                ImGui::EndMenu();
            }

            // TODO: Related to anim graphs only
            // if (ImGui::MenuItem("Compile & Save"))
            //{
            //    Compile();

            //    // TODO: Save to a common folder with the rest of the editor ?
            //    nlohmann::json json;
            //    SaveState(json);
            //    std::ofstream outputStream(m_statePath);
            //    outputStream << json;
            //}
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_pin_popup"))
        {
            ImGui::MenuItem("Pin !");
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_node_popup"))
        {
            // TODO: Factorize, we're looking for the node twice.
            // but we need to correctly reset the context menu if the node is deleted
            auto pNode = *std::find_if(m_pGraph->m_graphNodes.begin(), m_pGraph->m_graphNodes.end(), [&](auto pNode)
                { return pNode->GetID() == m_contextPopupElementID; });

            if (ImGui::MenuItem("Remove node"))
            {
                m_pGraph->RemoveGraphNode(m_contextPopupElementID);
                pNode = nullptr;
            }

            if (pNode != nullptr && pNode->SupportsDynamicInputPins() && ImGui::MenuItem("Add input pin"))
            {
                m_pGraph->AddDynamicInputPin(pNode);
            }

            if (pNode != nullptr && pNode->SupportsDynamicOutputPins() && ImGui::MenuItem("Add output pin"))
            {
                // TODO
            }

            // auto pStateNode = dynamic_cast<StateEditorNode*>(pNode);
            // if (pStateNode != nullptr)
            //{
            //     // TODO: New transition
            //     if (ImGui::MenuItem("Add transition"))
            //     {
            //     }
            // }

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_link_popup"))
        {
            if (ImGui::MenuItem("Remove link"))
            {
                m_pGraph->RemoveLink(m_contextPopupElementID);
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_link_dropped_popup"))
        {
            const ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
            auto pSelectedNodeType = m_pGraph->AvailableNodeTypesMenuItems(pTypeRegistryService);
            if (pSelectedNodeType != nullptr)
            {
                // TODO: These are never freed. Change AddGraphNode to accept a node's typeInfo ?
                auto pNode = pSelectedNodeType->CreateTypeInstance<EditorGraphNode>();
                pNode->Initialize();

                m_pGraph->AddGraphNode(pNode);

                ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
            }
            ImGui::EndPopup();
        }

        // Draw nodes
        for (auto pNode : m_pGraph->m_graphNodes)
        {
            pNode->DrawNode(drawingContext);
        }

        // Draw links
        for (auto& link : m_pGraph->m_links)
        {
            const auto pPin = m_pGraph->m_pinLookupMap[link.m_inputPinID];

            ImNodes::PushColorStyle(ImNodesCol_Link, drawingContext.GetTypeColor(pPin->GetValueType()).U32());
            ImNodes::Link(link.m_id, link.m_inputPinID, link.m_outputPinID);
            ImNodes::PopColorStyle();
        }

        ImNodes::EndNodeEditor();

        // if (ImGui::BeginDragDropTarget())
        // {
        //     if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("AssetID", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        //     {
        //         assert(pPayload->DataSize == sizeof(AssetID));
        //         auto pID = (AssetID*) pPayload->Data;
        //         if (pID->GetAssetTypeID() == AnimationGraphDefinition::GetStaticAssetTypeID())
        //         {
        //             auto pAssetService = context.GetService<AssetService>();
        //             pAssetService->Load(m_)
        //         }
        //         if (pID->GetAssetTypeID() == AnimationGraphDataset::GetStaticAssetTypeID())
        //         {
        //         }
        //     }
        //     ImGui::EndDragDropTarget();
        // }

        // Handle link creation
        UUID startPinID, endPinID, startNodeID, endNodeID;
        if (ImNodes::IsLinkCreated(&startNodeID, &startPinID, &endNodeID, &endPinID))
        {
            m_pGraph->AddLink(startNodeID, startPinID, endNodeID, endPinID);
        }

        UUID linkID;
        if (ImNodes::IsLinkDestroyed(&linkID))
        {
            m_pGraph->RemoveLink(linkID);
        }

        UUID pinID;
        if (ImNodes::IsLinkDropped(&pinID, false))
        {
            // TODO: Filter available node types matching the existing pin's type
            // TODO: Automatically create a link
            // ImGui::OpenPopup("graph_editor_link_dropped_popup");
        }
    }

    void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
    {
    }

    void SaveState(nlohmann::json& json) const
    {
    }
};

} // namespace aln