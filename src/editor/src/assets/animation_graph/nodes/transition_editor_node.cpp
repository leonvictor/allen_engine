#include "assets/animation_graph/nodes/transition_editor_node.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/transition.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, TransitionEditorNode)
ALN_REGISTER_IMPL_END()

void TransitionEditorNode::Initialize()
{
    m_name = "Transition";
    AddInputPin(NodeValueType::Bool, "Condition");
}

NodeIndex TransitionEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
{
    TransitionRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<TransitionRuntimeNode>(this, pGraphDefinition, pSettings);
    if (!compiled)
    {
        // TODO
    }

    return pSettings->GetNodeIndex();
};
} // namespace aln