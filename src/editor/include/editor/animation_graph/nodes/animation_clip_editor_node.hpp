#pragma once

#include <assets/asset_id.hpp>
#include <reflection/reflection.hpp>

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

    virtual void Serialize() override
    {
        // TODO
    }

    virtual void Initialize() override;
    virtual void Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};
} // namespace aln