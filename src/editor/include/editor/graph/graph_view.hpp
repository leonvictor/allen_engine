#pragma once

#include "graph_drawing_context.hpp"

#include <common/event.hpp>
#include <common/maths/vec2.hpp>
#include <common/serialization/json.hpp>

#include <imnodes.h>

namespace aln
{

class EditorGraph;
class EditorGraphNode;
class EditorAnimationStateMachine;
class Conduit;
class TypeRegistryService;
class Pin;
class Link;

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
    ~GraphView();

    void SetViewedGraph(EditorGraph* pGraph);

    // TODO: Use base classes to further decouple view from anim stuff
    bool IsViewingAnimationGraph() const;
    bool IsViewingStateMachine() const;

    bool HasGraphSet() const { return m_pGraph != nullptr; }
    EditorGraph* GetViewedGraph() const { return m_pGraph; }

    void Clear();

    // ---- Drawing
    void DrawNodeContextPopUp();
    void DrawCanvasContextPopUp(const TypeRegistryService* pTypeRegistryService, GraphDrawingContext& context);

    // TODO: Move registry in the context ?
    // TODO: Should context be shared between view instances ?
    void Draw(const TypeRegistryService* pTypeRegistryService, GraphDrawingContext& drawingContext);

    bool IsMouseDragging() const;

    void ClearLinkSelection();
    void ClearNodeSelection();

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
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService);
    void SaveState(JSON& json) const;
};

} // namespace aln