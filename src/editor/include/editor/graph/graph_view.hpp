#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "graph/editor_graph.hpp"
#include "graph/editor_graph_node.hpp"

namespace aln
{

/// @brief Stateful view of an editor graph
class GraphView
{
  private:
    EditorGraph* m_pGraph = nullptr;
    UUID m_contextPopupElementID = UUID::InvalidID;

    std::vector<UUID> m_selectedNodeIDs;
    std::vector<const EditorGraphNode*> m_selectedNodes;

    std::vector<UUID> m_selectedLinkIDs;
    std::vector<const Link*> m_selectedLinks;

    ImNodesContext* m_pImNodesContext = ImNodes::CreateContext();

  public:
    ~GraphView()
    {
        ImNodes::DestroyContext(m_pImNodesContext);
    }

    void SetViewedGraph(EditorGraph* pGraph)
    {
        if (pGraph == m_pGraph)
        {
            return;
        }

        if (pGraph != nullptr && pGraph->m_pImNodesEditorContext == nullptr)
        {
            pGraph->m_pImNodesEditorContext = ImNodes::EditorContextCreate();
            // TODO: Center on existing nodes when opening for the first time
        }
        
        m_pGraph = pGraph;
    }

    bool HasGraphSet() const { return m_pGraph != nullptr; }

    void Clear()
    {
        m_pGraph = nullptr;
        m_contextPopupElementID = UUID::InvalidID;
    }

    // TODO: Move registry in the context ?
    // TODO: Should context be shared between view instances ?
    void Draw(const TypeRegistryService* pTypeRegistryService, GraphDrawingContext& drawingContext)
    {
        if (m_pGraph == nullptr)
        {
            return;
        }

        ImNodes::SetCurrentContext(m_pImNodesContext);
        ImNodes::EditorContextSet(m_pGraph->m_pImNodesEditorContext);

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

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_pin_popup"))
        {
            // TODO: TMP
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
                ImNodes::ClearNodeSelection(m_contextPopupElementID);
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

        UUID destroyedlinkID;
        if (ImNodes::IsLinkDestroyed(&destroyedlinkID))
        {
            m_pGraph->RemoveLink(destroyedlinkID);
        }

        UUID linkDroppedPinID;
        if (ImNodes::IsLinkDropped(&linkDroppedPinID, false))
        {
            // TODO: Filter available node types matching the existing pin's type
            // TODO: Automatically create a link
            // ImGui::OpenPopup("graph_editor_link_dropped_popup");
        }

        // TODO: Cache selected nodes and links rather than re-populating the arrays each frame
        // Update selected nodes array
        auto selectedNodesCount = ImNodes::NumSelectedNodes();
        m_selectedNodeIDs.resize(selectedNodesCount);
        m_selectedNodes.resize(selectedNodesCount);
        if (selectedNodesCount != 0)
        {
            ImNodes::GetSelectedNodes(m_selectedNodeIDs.data());
            for (auto nodeIdx = 0; nodeIdx < selectedNodesCount; ++nodeIdx)
            {
                auto& nodeID = m_selectedNodeIDs[nodeIdx];
                m_selectedNodes[nodeIdx] = m_pGraph->GetNode(nodeID);
            }
        }

        auto selectedLinksCount = ImNodes::NumSelectedLinks();
        m_selectedLinkIDs.resize(selectedLinksCount);
        m_selectedLinks.resize(selectedLinksCount);
        if (selectedLinksCount != 0)
        {
            ImNodes::GetSelectedLinks(m_selectedLinkIDs.data());
            for (auto linkIdx = 0; linkIdx < selectedLinksCount; ++linkIdx)
            {
                auto& linkID = m_selectedLinkIDs[linkIdx];
                m_selectedLinks[linkIdx] = m_pGraph->GetLink(linkID);
            }
        }
    }

    // ------ Interactivity
    bool HasSelectedNodes() const { return !m_selectedNodes.empty(); }
    const std::vector<const EditorGraphNode*>& GetSelectedNodes() const { return m_selectedNodes; }

    bool HasSelectedLinks() const { return !m_selectedLinks.empty(); }
    const std::vector<const Link*>& GetSelectedLinks() const { return m_selectedLinks; }

    // ------ Serialization
    void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
    {
    }

    void SaveState(nlohmann::json& json) const
    {
    }
};

} // namespace aln