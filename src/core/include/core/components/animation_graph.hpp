#pragma once

#include <anim/animation_clip.hpp>
#include <anim/graph/graph.hpp>
#include <anim/skeleton.hpp>

#include <assets/handle.hpp>
#include <entities/component.hpp>

#include <assets/type_descriptors/handles.hpp>

#include <memory>

namespace aln
{
class AnimationGraphComponent : public entities::IComponent
{
    ALN_REGISTER_TYPE();

  private:
    std::shared_ptr<AssetManager> m_pAssetManager = nullptr;
    AssetHandle<AnimationClip> m_pAnimationClip; // Placeholder. TODO: Remove
    AssetHandle<Skeleton> m_pSkeleton;           // Animation skeleton
                                                 // TODO: AnimationGraphInstance

  public:
    // TODO: Should this be public ?
    void Construct(const entities::ComponentCreationContext& ctx) override
    {
        m_pAssetManager = ctx.pAssetManager;
        // TODO:
        m_pAnimationClip = ctx.pAssetManager->Get<AnimationClip>("assets/models/assets_export/robot/RobotArmature_Robot_Dance.anim");
    }

    void Initialize() override
    {
        m_pAssetManager->Initialize<AnimationClip>(m_pAnimationClip);
    }
    void Shutdown() override
    {
        m_pAssetManager->Shutdown<AnimationClip>(m_pAnimationClip);
    }
    bool Load() override
    {
        m_pAssetManager->Load<AnimationClip>(m_pAnimationClip);
        return true;
    }
    void Unload() override
    {
        m_pAssetManager->Unload<AnimationClip>(m_pAnimationClip);
    }
};
} // namespace aln