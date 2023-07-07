#pragma once

#include <assets/asset_id.hpp>
#include <reflection/type_info.hpp>

#include <anim/graph/nodes/animation_clip_node.hpp>

#include "../editor_graph_node.hpp"

namespace aln
{
/// @brief Node pointing to an animation clip used as graph input
class AnimationClipEditorNode : public EditorGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    AssetID m_animationClipID;

  public:
    const AssetID& GetAnimationClipID() { return m_animationClipID; }
    virtual bool IsRenamable() const override { return true; }

    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override
    {
        std::string assetPath = json["animation_clip"];
        m_animationClipID = AssetID(assetPath);
    }

    virtual void SaveState(nlohmann::json& json) const override
    {
        json["animation_clip"] = m_animationClipID.GetAssetPath();
    }

    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};
} // namespace aln