#include "assets/animation_graph/nodes/state_editor_node.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/state.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, StateEditorNode)
ALN_REGISTER_IMPL_END()

void StateEditorNode::Initialize()
{
    m_name = "State";
}

NodeIndex StateEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
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