#include "assets/animation_graph/nodes/animation_clip_editor_node.hpp"
#include "assets/animation_graph/animation_graph_compilation_context.hpp"

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, AnimationClipEditorNode)
ALN_REFLECT_MEMBER(m_animationClipID)
ALN_REGISTER_IMPL_END()

void AnimationClipEditorNode::Initialize()
{
    m_name = "Animation Clip";
    AddOutputPin(NodeValueType::Pose);
}

NodeIndex AnimationClipEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    AnimationClipRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<AnimationClipRuntimeNode>(this, graphDefinition, pSettings);
    if (!compiled)
    {
        pSettings->m_dataSlotIdx = context.RegisterDataSlot(GetID());
    }

    return pSettings->GetNodeIndex();
};

} // namespace aln
