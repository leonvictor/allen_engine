#pragma once

namespace aln
{

class StateEditorNode;

class EditorTransition
{
    friend class EditorAnimationStateMachine;
    friend class GraphView;

  private:
    const StateEditorNode* m_pStartState = nullptr;
    const StateEditorNode* m_pEndState = nullptr;
    // TODO: Child graph (= condition)
    // TODO: Compilation
};
} // namespace aln