#pragma once

#include "assets/animation_graph/editor_animation_graph.hpp"
#include "assets/animation_graph/nodes/transition_editor_node.hpp"

#include <anim/graph/nodes/transition.hpp>

namespace aln
{

class StateEditorNode;

/// @brief Unique directed conduit from one node to another
/// @todo Rename. EditorConduit ? Simply "Conduit" ?
// Conduits can hold multiple transitions between two nodes
class EditorTransition
{
    friend class EditorAnimationStateMachine;
    friend class GraphView;

  private:
    UUID m_id = UUID::Generate();
    const StateEditorNode* m_pStartState = nullptr;
    const StateEditorNode* m_pEndState = nullptr;

    EditorAnimationGraph* m_pChildGraph = nullptr;
    // TODO: Compilation

  public:
    void Shutdown()
    {
        aln::Delete(m_pChildGraph);
    }

    void Initialize()
    {
        m_pChildGraph = aln::New<EditorAnimationGraph>();

        // TODO: Rework node adding routines
        auto pTransitionNode = aln::New<TransitionEditorNode>();
        pTransitionNode->Initialize();
        m_pChildGraph->AddGraphNode(pTransitionNode);
    }

    const UUID& GetID() const { return m_id; }
    const StateEditorNode* GetStartState() const { return m_pStartState; }
    const StateEditorNode* GetEndState() const { return m_pEndState; }

    bool HasChildGraph() const { return m_pChildGraph != nullptr; }
    EditorAnimationGraph* GetChildGraph() const { return m_pChildGraph; }
    
    // void Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
    //{
    //     TransitionRuntimeNode::Settings* pSettings = nullptr;
    //     auto compiled = context.GetSettings<TransitionRuntimeNode>(this, pGraphDefinition, pSettings);
    // }
};
} // namespace aln