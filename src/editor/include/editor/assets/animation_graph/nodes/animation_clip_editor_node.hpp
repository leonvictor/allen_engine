#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

#include <assets/asset_id.hpp>
#include <reflection/type_info.hpp>

namespace aln
{
class TypeRegistryService;

/// @brief Node pointing to an animation clip used as graph input
class AnimationClipEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    AssetID m_animationClipID;

  public:
    const AssetID& GetAnimationClipID() const { return m_animationClipID; }
    bool IsRenamable() const override { return true; }

    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override;
    void SaveState(JSON& json) const override;

    void Initialize() override;
    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln