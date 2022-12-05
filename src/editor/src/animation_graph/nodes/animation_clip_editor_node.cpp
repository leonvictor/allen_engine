#include "animation_graph/nodes/animation_clip_editor_node.hpp"
#include "animation_graph/animation_graph_compilation_context.hpp"

namespace aln
{
void AnimationClipEditorNode::Initialize()
{
    m_name = "Animation Clip";
    AddOutputPin(PinValueType::Pose);
}

void AnimationClipEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
{
    AnimationClipRuntimeNode::Settings* pSettings = nullptr;
    auto compiled = context.GetSettings<AnimationClipRuntimeNode>(this, pGraphDefinition, pSettings);
    if (!compiled)
    {
        pSettings->m_dataSlotIdx = context.RegisterDataSlot(GetID());
    }
};

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODE, AnimationClipEditorNode)
ALN_REFLECT_MEMBER(m_animationClipID, Clip)
ALN_REGISTER_IMPL_END()

} // namespace aln
