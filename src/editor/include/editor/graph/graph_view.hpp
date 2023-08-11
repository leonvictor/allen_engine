#pragma once

#include "graph/editor_graph.hpp"
#include "graph/editor_graph_node.hpp"

#include "aln_imgui_widgets.hpp"
#include "assets/animation_graph/editor_animation_state_machine.hpp"
#include "assets/animation_graph/nodes/state_editor_node.hpp"

#include <common/event.hpp>

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
    static constexpr float TransitionArrowsOffset = 4.0f;

    struct TransitionDragState
    {
        bool m_dragging = false;
        const EditorGraphNode* m_pStartNode = nullptr;
        glm::vec2 m_startPosition;

        void Reset()
        {
            m_dragging = false;
            m_pStartNode = nullptr;
            m_startPosition = {0, 0};
        }
    };

    struct ContextPopupState
    {
        const EditorGraphNode* m_pNode = nullptr;
        const Pin* m_pPin = nullptr;
        const Link* m_pLink = nullptr;
        const EditorTransition* m_pTransition = nullptr;
    };

  private:
    EditorGraph* m_pGraph = nullptr;

    std::vector<UUID> m_selectedNodeIDs;
    std::vector<const EditorGraphNode*> m_selectedNodes;

    std::vector<UUID> m_selectedLinkIDs;
    std::vector<const Link*> m_selectedLinks;

    // TODO: Handle selection of multiple transitions
    const EditorTransition* m_pSelectedTransition = nullptr;

    bool m_editorHovered = false;
    bool m_canvasHovered = false;
    const EditorTransition* m_pHoveredTransition = nullptr;
    const EditorGraphNode* m_pHoveredNode = nullptr;
    const Pin* m_pHoveredPin = nullptr;
    const Link* m_pHoveredLink = nullptr;

    ImNodesContext* m_pImNodesContext = ImNodes::CreateContext();

    TransitionDragState m_transitionDragState;
    ContextPopupState m_contextPopupState;

    Event<const EditorGraph*> m_canvasDoubleClickedEvent;
    Event<const EditorGraphNode*> m_nodeDoubleClickedEvent;
    Event<const EditorTransition*> m_transitionDoubleClickedEvent;
    // Links could also fire events, but its not needed right now

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
    EditorGraph* GetViewedGraph() const { return m_pGraph; }

    void Clear()
    {
        m_pGraph = nullptr;
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

        m_pHoveredTransition = nullptr;

        ImGui::PushID(this);
        ImNodes::BeginNodeEditor();

        m_editorHovered = ImNodes::IsEditorHovered();

        // Open contextual popups
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImNodes::IsEditorHovered())
        {
            if (IsPinHovered())
            {
                m_contextPopupState.m_pPin = m_pHoveredPin;
                ImGui::OpenPopup("graph_editor_pin_popup");
            }
            else if (IsNodeHovered())
            {
                m_contextPopupState.m_pNode = m_pHoveredNode;
                ImGui::OpenPopup("graph_editor_node_popup");
            }
            else if (IsLinkHovered())
            {
                m_contextPopupState.m_pLink = m_pHoveredLink;
                ImGui::OpenPopup("graph_editor_link_popup");
            }
            else
            {
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
            // This is not ideal... But the context menu is an exception that can alter the hovered node
            auto pNode = const_cast<EditorGraphNode*>(m_contextPopupState.m_pNode);

            if (ImGui::MenuItem("Remove node"))
            {
                if (ImNodes::IsNodeSelected(pNode->GetID()))
                {
                    ImNodes::ClearNodeSelection(pNode->GetID());
                }

                m_pGraph->RemoveGraphNode(pNode->GetID());
                m_contextPopupState.m_pNode = nullptr;
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

            auto pStateNode = dynamic_cast<const StateEditorNode*>(pNode);
            if (pStateNode != nullptr && IsViewingStateMachine() && ImGui::MenuItem("Transition to..."))
            {
                m_transitionDragState.m_dragging = true;
                m_transitionDragState.m_pStartNode = pStateNode;
                m_transitionDragState.m_startPosition = GetNodeScreenSpaceCenter(pStateNode->GetID());
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_link_popup"))
        {
            if (ImGui::MenuItem("Remove link"))
            {
                m_pGraph->RemoveLink(m_pHoveredLink->GetID());
                m_pHoveredLink = nullptr;
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_link_dropped_popup"))
        {
            const ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
            auto pSelectedNodeType = m_pGraph->AvailableNodeTypesMenuItems(pTypeRegistryService);
            if (pSelectedNodeType != nullptr)
            {
                // TODO: Change AddGraphNode to accept a node's typeInfo ?
                auto pNode = pSelectedNodeType->CreateTypeInstance<EditorGraphNode>();
                pNode->Initialize();

                m_pGraph->AddGraphNode(pNode);

                ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
            }
            ImGui::EndPopup();
        }

        if (IsViewingStateMachine())
        {
            auto pStateMachine = static_cast<EditorAnimationStateMachine*>(m_pGraph);

            if (IsDraggingTransition())
            {
                if (ImGui::IsAnyMouseDown())
                {
                    if (IsNodeHovered())
                    {
                        const auto pEndNode = m_pHoveredNode;
                        if (IsTransitionCreationAllowed(m_transitionDragState.m_pStartNode, pEndNode))
                        {
                            const auto pStartState = static_cast<const StateEditorNode*>(m_transitionDragState.m_pStartNode);
                            const auto pEndState = static_cast<const StateEditorNode*>(pEndNode);
                            auto pTransition = pStateMachine->CreateTransition(pStartState, pEndState);

                            ClearLinkSelection();
                            ClearNodeSelection();
                            m_transitionDragState.Reset();
                            m_pSelectedTransition = pTransition;
                        }
                    }

                    // Drop the ongoing transition
                    m_transitionDragState.Reset();
                }
                else
                {
                    glm::vec2 endPosition = ImGui::GetMousePos();

                    if (IsNodeHovered())
                    {
                        // Snap to state nodes
                        const auto pEndNode = m_pHoveredNode;
                        if (IsTransitionCreationAllowed(m_transitionDragState.m_pStartNode, pEndNode))
                        {
                            auto hoveredNodeCenter = GetNodeScreenSpaceCenter(pEndNode->GetID());
                            endPosition = GetNodeBorderIntersection(pEndNode->GetID(), m_transitionDragState.m_startPosition, hoveredNodeCenter);
                        }
                    }

                    auto direction = glm::normalize(endPosition - m_transitionDragState.m_startPosition);
                    glm::vec2 orthogonalDirection = {-direction.y, direction.x};

                    auto startPosition = GetNodeBorderIntersection(m_transitionDragState.m_pStartNode->GetID(), endPosition, m_transitionDragState.m_startPosition) + orthogonalDirection * TransitionArrowsOffset;
                    if (IsNodeHovered())
                    {
                        endPosition = endPosition + TransitionArrowsOffset * orthogonalDirection;
                    }

                    ImGuiWidgets::DrawArrow(m_pImNodesContext->CanvasDrawList, startPosition, endPosition, IM_COL32_BLACK);
                }
            }

            // Draw transitions
            for (auto& pTransition : pStateMachine->m_transitions)
            {
                const auto& startStateID = pTransition->m_pStartState->GetID();
                const auto& endStateID = pTransition->m_pEndState->GetID();

                auto startStateCenter = GetNodeScreenSpaceCenter(startStateID);
                auto endStateCenter = GetNodeScreenSpaceCenter(endStateID);

                auto direction = glm::normalize(endStateCenter - startStateCenter);
                glm::vec2 orthogonalDirection = {-direction.y, direction.x};

                auto startPosition = GetNodeBorderIntersection(startStateID, endStateCenter, startStateCenter) + TransitionArrowsOffset * orthogonalDirection;
                auto endPosition = GetNodeBorderIntersection(endStateID, startStateCenter, endStateCenter) + TransitionArrowsOffset * orthogonalDirection;

                auto transitionColor = RGBColor::Blue;
                // Handle hovering and selection
                glm::vec2 mousePosition = ImGui::GetMousePos();
                glm::vec2 closestPointOnTransitionFromMouse = ImLineClosestPoint(startPosition, endPosition, ImGui::GetMousePos());
                auto distanceToTransition = glm::length2(mousePosition - closestPointOnTransitionFromMouse);

                if (distanceToTransition < 25.0f)
                {
                    m_pHoveredTransition = pTransition;
                    transitionColor = RGBColor::Green;
                    // TODO: Change color

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        ClearLinkSelection();
                        ClearNodeSelection();
                        m_pSelectedTransition = pTransition;
                    }
                }

                transitionColor = (pTransition == m_pSelectedTransition) ? RGBColor::Pink : transitionColor;
                ImGuiWidgets::DrawArrow(m_pImNodesContext->CanvasDrawList, startPosition, endPosition, transitionColor.U32());
            }
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

        // Reset hovered state
        UUID id = UUID::InvalidID;
        m_pHoveredNode = ImNodes::IsNodeHovered(&id) ? m_pGraph->GetNode(id) : nullptr;
        m_pHoveredPin = ImNodes::IsPinHovered(&id) ? m_pGraph->GetPin(id) : nullptr;
        m_pHoveredLink = ImNodes::IsLinkHovered(&id) ? m_pGraph->GetLink(id) : nullptr;

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

        // -------- Selection
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

        if (m_editorHovered && !IsTransitionHovered() && ImGui::IsAnyMouseDown())
        {
            m_pSelectedTransition = nullptr;
        }

        // -------- Navigation

        if (m_editorHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (IsNodeHovered())
            {
                m_nodeDoubleClickedEvent.Fire(m_pHoveredNode);
            }
            else if (IsTransitionHovered())
            {
                m_transitionDoubleClickedEvent.Fire(m_pHoveredTransition);
            }
            else if (IsLinkHovered())
            {
                // ...
            }
            else if (IsPinHovered())
            {
                // ...
            }
            else
            {
                m_canvasDoubleClickedEvent.Fire(m_pGraph);
            }
        }

        ImGui::PopID();
    }

    // ------ Helpers
    // TODO: Move to private
    glm::vec2 GetNodeScreenSpaceCenter(const UUID& nodeID) const
    {
        return (glm::vec2) ImNodes::GetNodeScreenSpacePos(nodeID) + (glm::vec2) ImNodes::GetNodeDimensions(nodeID) / 2.0f;
    }

    glm::vec2 GetNodeBorderIntersection(const UUID& nodeID, glm::vec2& start, glm::vec2& end) const
    {
        /// @note AABB Intersection https://noonat.github.io/intersect/#aabb-vs-segment
        auto nodeCenter = GetNodeScreenSpaceCenter(nodeID);
        glm::vec2 nodeHalf = (glm::vec2) ImNodes::GetNodeDimensions(nodeID) / 2.0f;

        const auto delta = end - start;
        const auto scale = 1.0f / delta;
        const auto sign = glm::sign(scale);
        const auto nearTimes = (nodeCenter - sign * nodeHalf - start) * scale;
        const auto farTimes = (nodeCenter + sign * nodeHalf - start) * scale;

        assert(!(nearTimes.x > farTimes.y || nearTimes.y > farTimes.x)); // No collision

        const auto nearTime = glm::max(nearTimes.x, nearTimes.y);
        const auto farTime = glm::min(farTimes.x, farTimes.y);

        assert(nearTime < 1.0f); // Segment goes toward the AABB but does not reach it (TODO: Raycast ?)
        assert(farTime > 0.0f);  // Segment points away from the AABB

        float hitTime = std::clamp<float>(nearTime, 0.0f, 1.0f);
        // auto hitDelta = (1.0f - hitTime) * -delta;
        auto hitPos = start + delta * hitTime;

        return hitPos;
    }

    bool IsTransitionCreationAllowed(const EditorGraphNode* pStartNode, const EditorGraphNode* pEndNode) const
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
        return true;
    }

    bool IsMouseDragging() const { return m_pImNodesContext->LeftMouseDragging; }

    void ClearLinkSelection()
    {
        ImNodes::ClearLinkSelection();
        m_selectedLinkIDs.clear();
        m_selectedLinks.clear();
    }

    void ClearNodeSelection()
    {
        ImNodes::ClearNodeSelection();
        m_selectedNodeIDs.clear();
        m_selectedNodes.clear();
    }

    // ------ Interactions
    bool HasSelectedNodes() const { return !m_selectedNodes.empty(); }
    const std::vector<const EditorGraphNode*>& GetSelectedNodes() const { return m_selectedNodes; }

    bool HasSelectedLinks() const { return !m_selectedLinks.empty(); }
    const std::vector<const Link*>& GetSelectedLinks() const { return m_selectedLinks; }

    bool HasSelectedTransition() const { return m_pSelectedTransition != nullptr; }
    const EditorTransition* GetSelectedTransition() const { return m_pSelectedTransition; }

    bool IsDraggingTransition() const { return m_transitionDragState.m_dragging; }

    bool IsNodeHovered() const { return m_pHoveredNode != nullptr; }
    bool IsLinkHovered() const { return m_pHoveredLink != nullptr; }
    bool IsPinHovered() const { return m_pHoveredPin != nullptr; }
    bool IsTransitionHovered() const { return m_pHoveredTransition != nullptr; }

    Event<const EditorGraph*>& OnCanvasDoubleClicked() { return m_canvasDoubleClickedEvent; }
    Event<const EditorGraphNode*>& OnNodeDoubleClicked() { return m_nodeDoubleClickedEvent; }
    Event<const EditorTransition*>& OnTransitionDoubleClicked() { return m_transitionDoubleClickedEvent; }

    // ------ Serialization
    void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
    {
    }

    void SaveState(nlohmann::json& json) const
    {
    }
};

} // namespace aln