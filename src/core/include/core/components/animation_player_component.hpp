#pragma once

#include <anim/animation_clip.hpp>
#include <anim/pose.hpp>
#include <anim/skeleton.hpp>

#include <assets/handle.hpp>
#include <entities/component.hpp>

#include <assets/type_descriptors/handles.hpp>

#include <common/types.hpp>

#include <math.h>
#include <memory>

namespace aln
{

class AnimationPlayerComponent : public entities::IComponent
{
    ALN_REGISTER_TYPE();

  private:
    std::shared_ptr<AssetManager> m_pAssetManager = nullptr;

    AssetHandle<Skeleton> m_pSkeleton; // Animation skeleton
    AssetHandle<AnimationClip> m_pAnimationClip;

    Pose* m_pPose = nullptr;
    Percentage m_previousAnimTime = 0.0f;
    Percentage m_animTime = 0.0f;

  public:
    inline const Pose* GetPose()     {         return m_pPose;     }

    void Update(Seconds deltaTime);

    // TODO: Should this be public ?
    void Construct(const entities::ComponentCreationContext& ctx) override
    {
        m_pAssetManager = ctx.pAssetManager;
        // TODO:

        // m_pSkeleton = ctx.pAssetManager->Get<Skeleton>("D:/Dev/allen_engine/assets/models/assets_export/Mike/RobotArmature.skel"); // TODO
        // m_pAnimationClip = ctx.pAssetManager->Get<AnimationClip>("D:/Dev/allen_engine/assets/models/assets_export/Mike/Hello.anim");

        m_pSkeleton = ctx.pAssetManager->Get<Skeleton>("D:/Dev/allen_engine/assets/models/assets_export/CesiumMan/Armature.skel"); // TODO
        m_pAnimationClip = ctx.pAssetManager->Get<AnimationClip>("D:/Dev/allen_engine/assets/models/assets_export/CesiumMan/Default.anim");
    }

    void Initialize() override
    {
        m_pAssetManager->Initialize<AnimationClip>(m_pAnimationClip);
        m_pAssetManager->Initialize<Skeleton>(m_pSkeleton);
        m_pPose = new Pose(m_pSkeleton.get()); // TODO
    }

    void Shutdown() override
    {
        delete m_pPose;
        m_pAssetManager->Shutdown<Skeleton>(m_pSkeleton);
        m_pAssetManager->Shutdown<AnimationClip>(m_pAnimationClip);
    }

    bool Load() override
    {
        m_pAssetManager->Load<AnimationClip>(m_pAnimationClip);
        m_pAssetManager->Load<Skeleton>(m_pSkeleton);
        return true;
    }

    void Unload() override
    {
        m_pAssetManager->Unload<AnimationClip>(m_pAnimationClip);
        m_pAssetManager->Unload<Skeleton>(m_pSkeleton);
    }
};
} // namespace aln