#pragma once

#include "graph/editor_graph.hpp"
#include "graph/editor_graph_node.hpp"

#include "assets/animation_graph/editor_animation_state_machine.hpp"
#include "assets/animation_graph/nodes/state_editor_node.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <imnodes.h>
#include <imnodes_internal.h>

#include <algorithm>

namespace aln
{

/// @brief Stateful view of an editor graph
class GraphView
{
    struct TransitionDragState
    {
        bool m_dragging = false;
        EditorGraphNode* m_pStartNode = nullptr;
        glm::vec2 m_startPosition;

        void Reset()
        {
            m_dragging = false;
            m_pStartNode = nullptr;
            m_startPosition = {0, 0};
        }
    };

  private:
    EditorGraph* m_pGraph = nullptr;
    UUID m_contextPopupElementID = UUID::InvalidID;

    std::vector<UUID> m_selectedNodeIDs;
    std::vector<const EditorGraphNode*> m_selectedNodes;

    std::vector<UUID> m_selectedLinkIDs;
    std::vector<const Link*> m_selectedLinks;

    UUID m_hoveredNodeID;
    UUID m_hoveredPinID;
    UUID m_hoveredLinkID;

    ImNodesContext* m_pImNodesContext = ImNodes::CreateContext();

    TransitionDragState m_transitionDragState;

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

    // TODO: Use base classes to further decouple view from anim stuff
    bool IsViewingAnimationGraph() const { return m_pGraph != nullptr && dynamic_cast<EditorAnimationGraph*>(m_pGraph) != nullptr; }
    bool IsViewingStateMachine() const { return m_pGraph != nullptr && dynamic_cast<EditorAnimationStateMachine*>(m_pGraph) != nullptr; }

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

        bool nodeHovered = ImNodes::IsNodeHovered(&m_hoveredNodeID);
        bool pinHovered = ImNodes::IsPinHovered(&m_hoveredPinID);
        bool linkHovered = ImNodes::IsLinkHovered(&m_hoveredLinkID);

        ImNodes::BeginNodeEditor();

        if (IsDraggingTransition())
        {
            if (ImGui::IsAnyMouseDown())
            {
                if (nodeHovered)
                {
                    auto pEndNode = m_pGraph->GetNode(m_hoveredNodeID);
                    if (IsTransitionCreationAllowed(m_transitionDragState.m_pStartNode, pEndNode))
                    {
                        // TODO: Create transition
                    }
                }
                // Drop the ongoing transition
                m_transitionDragState.Reset();
            }
            else
            {
                glm::vec2 endPosition = ImGui::GetMousePos();
                if (nodeHovered)
                {
                    // Snap to state nodes
                    // TODO: Only snap when the transition is possible
                    // TODO: Account for multiple transitions between the same states
                    auto pEndNode = m_pGraph->GetNode(m_hoveredNodeID);
                    if (IsTransitionCreationAllowed(m_transitionDragState.m_pStartNode, pEndNode))
                    {
                        auto hoveredNodeCenter = GetNodeScreenSpaceCenter(m_hoveredNodeID);
                        endPosition = GetNodeBorderIntersection(m_hoveredNodeID, m_transitionDragState.m_startPosition, hoveredNodeCenter);
                    }
                }
                std::cout << endPosition.x << ", " << endPosition.y << std::endl;

                // Draw the transition
                m_pImNodesContext->CanvasDrawList->AddLine(
                    m_transitionDragState.m_startPosition,
                    endPosition, IM_COL32_BLACK, 2.0f);
                
                // TODO: Draw arrow
                auto direction = glm::normalize(endPosition - m_transitionDragState.m_startPosition);
                glm::vec2 orthogonal = {-direction.y, direction.x};
                glm::vec2 triangleBase = endPosition - (direction * 16.0f);
                glm::vec2 point1 = triangleBase + (orthogonal * 8.0f);
                glm::vec2 point2 = triangleBase - (orthogonal * 8.0f);
                glm::vec2 point3 = endPosition;
                m_pImNodesContext->CanvasDrawList->AddTriangleFilled(point1, point2, point3, IM_COL32_BLACK);
            }
        }

        // Open contextual popups and register context ID
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImNodes::IsEditorHovered())
        {
            if (pinHovered)
            {
                m_contextPopupElementID = m_hoveredPinID;
                ImGui::OpenPopup("graph_editor_pin_popup");
            }
            else if (nodeHovered)
            {
                m_contextPopupElementID = m_hoveredNodeID;
                ImGui::OpenPopup("graph_editor_node_popup");
            }
            else if (linkHovered)
            {
                m_contextPopupElementID = m_hoveredLinkID;
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

            // TODO: Only when in state machines
            auto pStateNode = dynamic_cast<StateEditorNode*>(pNode);
            if (pStateNode != nullptr)
            {
                // TODO: New transition
                if (ImGui::MenuItem("Transition to..."))
                {
                    m_transitionDragState.m_dragging = true;
                    m_transitionDragState.m_pStartNode = pStateNode;
                    m_transitionDragState.m_startPosition = GetNodeScreenSpaceCenter(m_contextPopupElementID);
                }
            }

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

    // ------ Helpers
    glm::vec2 GetNodeScreenSpaceCenter(const UUID& nodeID) const
    {
        return ImNodes::GetNodeScreenSpacePos(nodeID) + ImNodes::GetNodeDimensions(nodeID) / 2;
    }

    glm::vec2 GetNodeBorderIntersection(const UUID& nodeID, glm::vec2& start, glm::vec2& end) const
    {
        /// @note AABB Intersection https://noonat.github.io/intersect/#aabb-vs-segment
        auto nodeCenter = GetNodeScreenSpaceCenter(nodeID);
        auto nodeHalf = ImNodes::GetNodeDimensions(nodeID) / 2;
        
        const auto delta = end - start;
        const auto scale = 1.0f / delta;
        const auto sign = glm::sign(scale);
        const auto nearTimes = (nodeCenter - sign * nodeHalf - start) * scale; 
        const auto farTimes = (nodeCenter + sign * nodeHalf - start) * scale;

        assert(!(nearTimes.x > farTimes.y || nearTimes.y > farTimes.x)); // No collision

        const auto nearTime = glm::max(nearTimes.x, nearTimes.y);
        const auto farTime = glm::min(farTimes.x, farTimes.y);

        assert(nearTime < 1.0f); // Segment goes toward the AABB but does not reach it (TODO: Raycast ?)
        assert(farTime > 0.0f); // Segment points away from the AABB 

        float hitTime = std::clamp<float>(nearTime, 0.0f, 1.0f);
        //auto hitDelta = (1.0f - hitTime) * -delta;
        auto hitPos = start + delta * hitTime;
        
        return hitPos;
    }

    bool IsTransitionCreationAllowed(const EditorGraphNode* pStartNode, const EditorGraphNode* pEndNode)
    {
        assert(pStartNode != nullptr && pEndNode != nullptr);

        if (pStartNode == pEndNode)
        {
            return false;
        }

        const auto pStartState = dynamic_cast<const StateEditorNode*>(pStartNode);
        const auto pEndState = dynamic_cast<const StateEditorNode*>(pEndNode);
        assert(pStartState != nullptr && pEndState != nullptr); // Both nodes must be state nodes

        // TODO:
        // - No existing identical link
        std::cout << "Transition allowed" << std::endl;
        return true;
    }

    // ------ Interactions
    bool HasSelectedNodes() const { return !m_selectedNodes.empty(); }
    const std::vector<const EditorGraphNode*>& GetSelectedNodes() const { return m_selectedNodes; }

    bool HasSelectedLinks() const { return !m_selectedLinks.empty(); }
    const std::vector<const Link*>& GetSelectedLinks() const { return m_selectedLinks; }

    bool IsDraggingTransition() const { return m_transitionDragState.m_dragging; }

    // ------ Serialization
    void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
    {
    }

    void SaveState(nlohmann::json& json) const
    {
    }
};

} // namespace aln