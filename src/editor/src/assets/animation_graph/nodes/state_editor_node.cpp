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
    SetChildGraph(pChildGraph);
    pChildGraph->CreateGraphNode<PoseEditorNode>();
}

NodeIndex StateEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    StateRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<StateRuntimeNode>(this, graphDefinition, pSettings);
    if (!compiled)
    {
        auto pChildBlendTree = static_cast<const EditorAnimationGraph*>(GetChildGraph());
        pSettings->m_childNodeIdx = pChildBlendTree->CompileDefinition(context, graphDefinition);
        if (pSettings->m_childNodeIdx == InvalidIndex)
        {
            context.LogError("There was an error while compiling the state's child graph", this);
            return InvalidIndex;
        }
    }
    return pSettings->GetNodeIndex();
};
} // namespace aln