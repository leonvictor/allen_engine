#include "assets/animation_graph/nodes/animation_clip_editor_node.hpp"
#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/animation_clip_node.hpp>
#include <common/serialization/json.hpp>

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

void AnimationClipEditorNode::LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService)
{
    std::string assetPath = json["animation_clip"];
    m_animationClipID = AssetID(assetPath);
}

void AnimationClipEditorNode::SaveState(JSON& json) const
{
    json["animation_clip"] = m_animationClipID.GetAssetPath();
}

} // namespace aln
