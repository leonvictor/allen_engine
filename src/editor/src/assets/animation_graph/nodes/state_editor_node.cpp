#include "assets/animation_graph/nodes/state_editor_node.hpp"

#include "assets/animation_graph/nodes/pose_editor_node.hpp"
#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/state.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, StateEditorNode)
ALN_REGISTER_IMPL_END()

void StateEditorNode::Initialize()
{
    m_name = "State";

    auto pChildGraph = aln::New<EditorAnimationGraph>();
    
    // TODO: Rework node creation routines
    auto pResultNode = aln::New<PoseEditorNode>();
    pResultNode->Initialize();
    pChildGraph->AddGraphNode(pResultNode);
    
    SetChildGraph(pChildGraph);
}

NodeIndex StateEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    StateRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<StateRuntimeNode>(this, pGraphDefinition, pSettings);
    if (!compiled)
    {
        // TODO
    }

    return pSettings->GetNodeIndex();
};
} // namespace aln