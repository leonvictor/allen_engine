#include "graph/conduit.hpp"

#include "assets/animation_graph/editor_animation_state_machine.hpp"

namespace aln
{

void Conduit::Initialize(EditorAnimationStateMachine* pOwningGraph)
{
    assert(pOwningGraph != nullptr);

    m_pChildGraph = aln::New<EditorAnimationGraph>();
    m_pChildGraph->Initialize(pOwningGraph);

    // TODO: Rework node adding routines
    auto pTransitionNode = aln::New<TransitionEditorNode>();
    pTransitionNode->Initialize();
    m_pChildGraph->AddGraphNode(pTransitionNode);
}

void Conduit::Shutdown()
{
    m_pChildGraph->Shutdown();
    aln::Delete(m_pChildGraph);
    m_pChildGraph = nullptr;
}
} // namespace aln