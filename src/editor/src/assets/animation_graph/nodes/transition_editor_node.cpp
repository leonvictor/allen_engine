#include "assets/animation_graph/nodes/transition_editor_node.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/transition.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, TransitionEditorNode)
ALN_REFLECT_MEMBER(m_duration);
ALN_REGISTER_IMPL_END()

void TransitionEditorNode::Initialize()
{
    m_name = "Transition";
    AddInputPin(NodeValueType::Bool, "Condition");
}

NodeIndex TransitionEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    assert(false); // Transition nodes compilation happens in their parent state machine's compilation method
    return InvalidIndex;
};
} // namespace aln