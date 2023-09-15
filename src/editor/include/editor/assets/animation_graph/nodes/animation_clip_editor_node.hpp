#pragma once

#include <assets/asset_id.hpp>
#include <reflection/type_info.hpp>

#include <anim/graph/nodes/animation_clip_node.hpp>

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{
/// @brief Node pointing to an animation clip used as graph input
class AnimationClipEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    AssetID m_animationClipID;

  public:
    const AssetID& GetAnimationClipID() const { return m_animationClipID; }
    virtual bool IsRenamable() const override { return true; }

    virtual void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override
    {
        std::string assetPath = json["animation_clip"];
        m_animationClipID = AssetID(assetPath);
    }

    virtual void SaveState(JSON& json) const override
    {
        json["animation_clip"] = m_animationClipID.GetAssetPath();
    }

    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln