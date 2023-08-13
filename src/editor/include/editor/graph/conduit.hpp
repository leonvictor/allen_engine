#pragma once

#include "assets/animation_graph/editor_animation_graph.hpp"
#include "assets/animation_graph/nodes/transition_editor_node.hpp"

#include <anim/graph/nodes/transition.hpp>

namespace aln
{

class StateEditorNode;

/// @brief Unique oriented conduit from one node to another
// Conduits can hold multiple transitions between the two nodes
class Conduit
{
    friend class EditorAnimationStateMachine;
    friend class GraphView;

  private:
    UUID m_id = UUID::Generate();
    const StateEditorNode* m_pStartState = nullptr;
    const StateEditorNode* m_pEndState = nullptr;

    EditorAnimationGraph* m_pChildGraph = nullptr;

  public:
    const UUID& GetID() const { return m_id; }
    const StateEditorNode* GetStartState() const { return m_pStartState; }
    const StateEditorNode* GetEndState() const { return m_pEndState; }

    bool HasChildGraph() const { return m_pChildGraph != nullptr; }
    EditorAnimationGraph* GetChildGraph() const { return m_pChildGraph; }

    void Initialize(EditorAnimationStateMachine* pOwningGraph);
    void Shutdown();
};
} // namespace aln