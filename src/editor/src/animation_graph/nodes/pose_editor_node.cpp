#include "animation_graph/nodes/pose_editor_node.hpp"
#include "animation_graph/animation_graph_compilation_context.hpp"

namespace aln
{
void PoseEditorNode::Initialize()
{
    m_name = "Output Pose";
    AddInputPin(NodeValueType::Pose, "Result");
}

NodeIndex PoseEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
{
    // Get the linked input node
    const auto& inputPin = GetInputPin(0);
    const auto pInputNode = context.GetNodeLinkedToInputPin(inputPin.GetID());

    // TODO: Handle, users will want to compile with malformed graphs,
    // we cannot afford to crash here !
    assert(pInputNode != nullptr);

    return pInputNode->Compile(context, pGraphDefinition); // The result is just a pointer to the previous node
};
} // namespace aln
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, aln::PoseEditorNode)
ALN_REGISTER_IMPL_END()