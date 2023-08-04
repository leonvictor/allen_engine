#include "animation_graph/nodes/state_machine_editor_node.hpp"

#include "animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/state_machine.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, StateMachineEditorNode)
ALN_REGISTER_IMPL_END()

void StateMachineEditorNode::Initialize()
{
    m_name = "State Machine";
}

NodeIndex StateMachineEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
{
    StateMachineRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<StateMachineRuntimeNode>(this, pGraphDefinition, pSettings);
    if (!compiled)
    {
       
    }

    return pSettings->GetNodeIndex();
};

} // namespace aln
