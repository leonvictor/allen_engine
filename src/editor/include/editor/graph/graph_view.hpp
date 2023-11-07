#pragma once

#include "graph/editor_graph.hpp"
#include "graph/editor_graph_node.hpp"

#include "aln_imgui_widgets.hpp"
#include "assets/animation_graph/editor_animation_state_machine.hpp"
#include "assets/animation_graph/nodes/control_parameter_editor_nodes.hpp"
#include "assets/animation_graph/nodes/parameter_reference_editor_node.hpp"
#include "assets/animation_graph/nodes/state_editor_node.hpp"
#include <common/maths/vec2.hpp>
#include <common/event.hpp>

#include <imgui.h>
#include <imnodes.h>
#include <imnodes_internal.h>

#include <algorithm>

namespace aln
{

/// @brief Stateful view of an editor graph
class GraphView
{
    static constexpr float ConduitArrowsOffset = 4.0f;

    struct ConduitDragState
    {
        bool m_dragging = false;
        const EditorGraphNode* m_pStartNode = nullptr;
        Vec2 m_startPosition;

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
        const Conduit* m_pConduit = nullptr;
    };

  private:
    EditorGraph* m_pGraph = nullptr;

    Vector<UUID> m_selectedNodeIDs;
    Vector<const EditorGraphNode*> m_selectedNodes;

    Vector<UUID> m_selectedLinkIDs;
    Vector<const Link*> m_selectedLinks;

    // TODO: Handle selection of multiple conduits
    const Conduit* m_pSelectedConduit = nullptr;

    bool m_editorHovered = false;
    bool m_canvasHovered = false;
    const Conduit* m_pHoveredConduit = nullptr;
    const EditorGraphNode* m_pHoveredNode = nullptr;
    const Pin* m_pHoveredPin = nullptr;
    const Link* m_pHoveredLink = nullptr;

    ImNodesContext* m_pImNodesContext = ImNodes::CreateContext();

    ConduitDragState m_conduitDragState;
    ContextPopupState m_contextPopupState;

    Event<const EditorGraph*> m_canvasDoubleClickedEvent;
    Event<const EditorGraphNode*> m_nodeDoubleClickedEvent;
    Event<const Conduit*> m_conduitDoubleClickedEvent;
    // Links could also fire events, but its not needed right now

    bool m_isFirstDraw = true;

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
        m_isFirstDraw = true;
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

    // ---- Drawing
    void DrawNodeContextPopUp()
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
            return;
        }

        if (pNode->IsRenamable() && ImGui::MenuItem("Rename..."))
        {
            pNode->BeginRenaming();
        }

        if (pNode->SupportsDynamicInputPins() && ImGui::MenuItem("Add input pin"))
        {
            m_pGraph->AddDynamicInputPin(pNode);
        }

        if (pNode->SupportsDynamicOutputPins() && ImGui::MenuItem("Add output pin"))
        {
            // TODO
        }

        auto pStateNode = dynamic_cast<const StateEditorNode*>(pNode);
        if (pStateNode != nullptr && IsViewingStateMachine() && ImGui::MenuItem("Conduit to..."))
        {
            m_conduitDragState.m_dragging = true;
            m_conduitDragState.m_pStartNode = pStateNode;
            m_conduitDragState.m_startPosition = GetNodeScreenSpaceCenter(pStateNode->GetID());
        }
    }

    void DrawCanvasContextPopUp(const TypeRegistryService* pTypeRegistryService, GraphDrawingContext& context)
    {
        const ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
        if (ImGui::BeginMenu("Add Node"))
        {
            auto pSelectedNodeType = m_pGraph->AvailableNodeTypesMenuItems(pTypeRegistryService);
            if (pSelectedNodeType != nullptr)
            {
                auto pNode = m_pGraph->CreateGraphNode(pSelectedNodeType);
                ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
            }
            if (ImGui::BeginMenu("Reference Parameter..."))
            {
                auto controlParameters = m_pGraph->GetRootGraph()->GetAllNodesOfType<IControlParameterEditorNode>(EditorGraph::NodeSearchScope::Recursive);
                for (auto& pParameter : controlParameters)
                {
                    if (ImGui::MenuItem(pParameter->GetName().c_str()))
                    {
                        auto pNode = m_pGraph->CreateGraphNode<ParameterReferenceEditorNode>(pParameter);
                        ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
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

        m_pHoveredConduit = nullptr;

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
            DrawCanvasContextPopUp(pTypeRegistryService, drawingContext);
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
            DrawNodeContextPopUp();
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
                auto pNode = m_pGraph->CreateGraphNode(pSelectedNodeType);
                ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
            }
            ImGui::EndPopup();
        }

        if (IsViewingStateMachine())
        {
            auto pStateMachine = static_cast<EditorAnimationStateMachine*>(m_pGraph);

            if (IsDraggingConduit())
            {
                if (ImGui::IsAnyMouseDown())
                {
                    if (IsNodeHovered())
                    {
                        const auto pEndNode = m_pHoveredNode;
                        if (IsConduitCreationAllowed(m_conduitDragState.m_pStartNode, pEndNode))
                        {
                            const auto pStartState = static_cast<const StateEditorNode*>(m_conduitDragState.m_pStartNode);
                            const auto pEndState = static_cast<const StateEditorNode*>(pEndNode);
                            auto pConduit = pStateMachine->CreateConduit(pStartState, pEndState);

                            ClearLinkSelection();
                            ClearNodeSelection();
                            m_conduitDragState.Reset();
                            m_pSelectedConduit = pConduit;
                        }
                    }

                    // Drop the ongoing conduit
                    m_conduitDragState.Reset();
                }
                else
                {
                    Vec2 endPosition = ImGui::GetMousePos();

                    if (IsNodeHovered())
                    {
                        // Snap to state nodes
                        const auto pEndNode = m_pHoveredNode;
                        if (IsConduitCreationAllowed(m_conduitDragState.m_pStartNode, pEndNode))
                        {
                            auto hoveredNodeCenter = GetNodeScreenSpaceCenter(pEndNode->GetID());
                            endPosition = GetNodeBorderIntersection(pEndNode->GetID(), m_conduitDragState.m_startPosition, hoveredNodeCenter);
                        }
                    }

                    auto direction = (endPosition - m_conduitDragState.m_startPosition).Normalized();
                    Vec2 orthogonalDirection = {-direction.y, direction.x};

                    auto startPosition = GetNodeBorderIntersection(m_conduitDragState.m_pStartNode->GetID(), endPosition, m_conduitDragState.m_startPosition) + orthogonalDirection * ConduitArrowsOffset;
                    if (IsNodeHovered())
                    {
                        endPosition = endPosition + ConduitArrowsOffset * orthogonalDirection;
                    }

                    ImGuiWidgets::DrawArrow(m_pImNodesContext->CanvasDrawList, startPosition, endPosition, IM_COL32_BLACK);
                }
            }

            // Draw conduits
            // Do not draw them on the first frame since the node dimensions might not be known
            if (!m_isFirstDraw)
            {

                for (auto& pConduit : pStateMachine->m_conduits)
                {
                    const auto& startStateID = pConduit->m_pStartState->GetID();
                    const auto& endStateID = pConduit->m_pEndState->GetID();

                    auto startStateCenter = GetNodeScreenSpaceCenter(startStateID);
                    auto endStateCenter = GetNodeScreenSpaceCenter(endStateID);

                    auto direction = (endStateCenter - startStateCenter).Normalized();
                    Vec2 orthogonalDirection = {-direction.y, direction.x};

                    auto startPosition = GetNodeBorderIntersection(startStateID, endStateCenter, startStateCenter) + ConduitArrowsOffset * orthogonalDirection;
                    auto endPosition = GetNodeBorderIntersection(endStateID, startStateCenter, endStateCenter) + ConduitArrowsOffset * orthogonalDirection;

                    auto conduitColor = RGBColor::Blue;
                    // Handle hovering and selection
                    Vec2 mousePosition = ImGui::GetMousePos();
                    Vec2 closestPointOnConduitFromMouse = ImLineClosestPoint(startPosition, endPosition, ImGui::GetMousePos());
                    auto distanceToConduit = (mousePosition - closestPointOnConduitFromMouse).SquaredMagnitude();

                    if (distanceToConduit < 25.0f)
                    {
                        m_pHoveredConduit = pConduit;
                        conduitColor = RGBColor::Green;
                        // TODO: Change color

                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            ClearLinkSelection();
                            ClearNodeSelection();
                            m_pSelectedConduit = pConduit;
                        }
                    }

                    conduitColor = (pConduit == m_pSelectedConduit) ? RGBColor::Pink : conduitColor;
                    ImGuiWidgets::DrawArrow(m_pImNodesContext->CanvasDrawList, startPosition, endPosition, static_cast<uint32_t>(conduitColor));
                }
            }
        }

        // Draw nodes
        for (auto pNode : m_pGraph->m_graphNodes)
        {
            pNode->DrawNode(drawingContext);
        }

        // Draw links
        for (const auto& link : m_pGraph->m_links)
        {
            const auto pPin = m_pGraph->m_pinLookupMap[link.m_inputPinID];
            auto colorScheme = drawingContext.GetTypeColorScheme(pPin->GetValueType());

            ImNodes::PushColorStyle(ImNodesCol_Link, static_cast<uint32_t>(colorScheme.m_defaultColor));
            ImNodes::PushColorStyle(ImNodesCol_LinkHovered, static_cast<uint32_t>(colorScheme.m_hoveredColor));
            ImNodes::PushColorStyle(ImNodesCol_LinkSelected, static_cast<uint32_t>(colorScheme.m_selectedColor));

            ImNodes::Link(link.m_id, link.m_inputPinID, link.m_outputPinID);
            
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
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

        if (m_editorHovered && !IsConduitHovered() && ImGui::IsAnyMouseDown())
        {
            m_pSelectedConduit = nullptr;
        }

        // -------- Navigation

        if (m_editorHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (IsNodeHovered())
            {
                m_nodeDoubleClickedEvent.Fire(m_pHoveredNode);
            }
            else if (IsConduitHovered())
            {
                m_conduitDoubleClickedEvent.Fire(m_pHoveredConduit);
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
        m_isFirstDraw = false;
    }

    // ------ Helpers
    // TODO: Move to private
    Vec2 GetNodeScreenSpaceCenter(const UUID& nodeID) const
    {
        return (Vec2) ImNodes::GetNodeScreenSpacePos(nodeID) + (Vec2) ImNodes::GetNodeDimensions(nodeID) / 2.0f;
    }

    Vec2 GetNodeBorderIntersection(const UUID& nodeID, Vec2& start, Vec2& end) const
    {
        /// @note AABB Intersection https://noonat.github.io/intersect/#aabb-vs-segment
        auto nodeCenter = GetNodeScreenSpaceCenter(nodeID);
        Vec2 nodeHalf = (Vec2) ImNodes::GetNodeDimensions(nodeID) / 2.0f;

        const auto delta = end - start;
        const auto scale = 1.0f / delta;
        const auto sign = scale.Sign();
        const auto nearTimes = scale.Scale((nodeCenter - sign.Scale(nodeHalf) - start));
        const auto farTimes = scale.Scale((nodeCenter + sign.Scale(nodeHalf) - start));

        assert(!(nearTimes.x > farTimes.y || nearTimes.y > farTimes.x)); // No collision

        const auto nearTime = Maths::Max(nearTimes.x, nearTimes.y);
        const auto farTime = Maths::Min(farTimes.x, farTimes.y);

        assert(nearTime < 1.0f); // Segment goes toward the AABB but does not reach it (TODO: Raycast ?)
        assert(farTime > 0.0f);  // Segment points away from the AABB

        float hitTime = Maths::Clamp(nearTime, 0.0f, 1.0f);
        // auto hitDelta = (1.0f - hitTime) * -delta;
        auto hitPos = start + delta * hitTime;

        return hitPos;
    }

    bool IsConduitCreationAllowed(const EditorGraphNode* pStartNode, const EditorGraphNode* pEndNode) const
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
    const Vector<const EditorGraphNode*>& GetSelectedNodes() const { return m_selectedNodes; }

    bool HasSelectedLinks() const { return !m_selectedLinks.empty(); }
    const Vector<const Link*>& GetSelectedLinks() const { return m_selectedLinks; }

    bool HasSelectedConduit() const { return m_pSelectedConduit != nullptr; }
    const Conduit* GetSelectedConduit() const { return m_pSelectedConduit; }

    bool IsDraggingConduit() const { return m_conduitDragState.m_dragging; }

    bool IsNodeHovered() const { return m_pHoveredNode != nullptr; }
    bool IsLinkHovered() const { return m_pHoveredLink != nullptr; }
    bool IsPinHovered() const { return m_pHoveredPin != nullptr; }
    bool IsConduitHovered() const { return m_pHoveredConduit != nullptr; }

    Event<const EditorGraph*>& OnCanvasDoubleClicked() { return m_canvasDoubleClickedEvent; }
    Event<const EditorGraphNode*>& OnNodeDoubleClicked() { return m_nodeDoubleClickedEvent; }
    Event<const Conduit*>& OnConduitDoubleClicked() { return m_conduitDoubleClickedEvent; }

    // ------ Serialization
    void LoadState(JSON& json, const TypeRegistryService* pTypeRegistryService)
    {
    }

    void SaveState(JSON& json) const
    {
    }
};

} // namespace aln