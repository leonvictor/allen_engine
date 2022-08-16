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

#include <assets/asset_service.hpp>

namespace aln
{

class AnimationPlayerComponent : public entities::IComponent
{
    ALN_REGISTER_TYPE();

  private:
    AssetHandle<Skeleton> m_pSkeleton; // Animation skeleton
    AssetHandle<AnimationClip> m_pAnimationClip;

    Pose* m_pPose = nullptr;
    Percentage m_previousAnimTime = 0.0f;
    Percentage m_animTime = 0.0f;

  public:
    inline const Pose* GetPose() { return m_pPose; }

    void Update(Seconds deltaTime);

    // TODO: Should this be public ?
    void Construct(const entities::ComponentCreationContext& ctx) override
    {
        m_pSkeleton = AssetHandle<Skeleton>("D:/Dev/allen_engine/assets/assets_export/CesiumMan/Armature.skel"); // TODO
        m_pAnimationClip = AssetHandle<AnimationClip>("D:/Dev/allen_engine/assets/assets_export/CesiumMan/Default.anim");
    }

    void Initialize() override
    {
        assert(m_pAnimationClip.IsLoaded());
        assert(m_pSkeleton.IsLoaded());
        m_pPose = new Pose(m_pSkeleton.get()); // TODO
    }

    void Shutdown() override
    {
        delete m_pPose;
    }

    void Load(const entities::LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Load(m_pAnimationClip);
        loadingContext.m_pAssetService->Load(m_pSkeleton);
    }

    void Unload(const entities::LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Unload(m_pAnimationClip);
        loadingContext.m_pAssetService->Unload(m_pSkeleton);
    }

    bool UpdateLoadingStatus() override
    {
        if (m_pAnimationClip.IsLoaded() && m_pSkeleton.IsLoaded())
        {
            m_status = Status::Loaded;
        }

        return IsLoaded();
    }
};
} // namespace aln