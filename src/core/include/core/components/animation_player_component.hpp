#pragma once

#include <anim/animation_clip.hpp>
#include <anim/pose.hpp>
#include <anim/skeleton.hpp>

#include <assets/handle.hpp>
#include <entities/component.hpp>

#include <common/types.hpp>

#include <math.h>
#include <memory>

#include <assets/asset_service.hpp>

namespace aln
{

class AnimationPlayerComponent : public IComponent
{
    ALN_REGISTER_TYPE();

  private:
    AssetHandle<Skeleton> m_pSkeleton; // Animation skeleton
    AssetHandle<AnimationClip> m_pAnimationClip;

    Pose* m_pPose = nullptr;
    Percentage m_previousAnimTime = 0.0f;
    Percentage m_animTime = 0.0f;

    // Editor
    // TODO: Move out / Pause the whole world rather than just an anim player
    bool m_pause = false;

  public:
    inline const Pose* GetPose() { return m_pPose; }
    inline const AssetHandle<AnimationClip>& GetAnimationClip() const { return m_pAnimationClip; }

    void Update(Seconds deltaTime);

    void SetSkeleton(const AssetHandle<Skeleton>& handle)
    {
        assert(IsUnloaded());
        m_pSkeleton = handle;
    }

    void SetAnimationClip(const AssetHandle<AnimationClip>& handle)
    {
        assert(IsUnloaded());
        m_pAnimationClip = handle;
    }

    void Initialize() override
    {
        assert(m_pAnimationClip.IsLoaded());
        assert(m_pSkeleton.IsLoaded());
        m_pPose = aln::New<Pose>(m_pSkeleton.get()); // TODO
    }

    void Shutdown() override
    {
        aln::Delete(m_pPose);
    }

    void Load(const LoadingContext& loadingContext) override
    {
        if (m_pAnimationClip.IsValid())
        {
        loadingContext.m_pAssetService->Load(m_pAnimationClip);
        }
    }

    void Unload(const LoadingContext& loadingContext) override
    {
        if (m_pAnimationClip.IsValid())
        {
        loadingContext.m_pAssetService->Unload(m_pAnimationClip);
        }
    }

    bool UpdateLoadingStatus() override
    {
        if (m_pAnimationClip.IsLoaded())
        {
            m_status = Status::Loaded;
        }
        else if (m_pAnimationClip.HasFailedLoading())
        {
            m_status = Status::LoadingFailed;
        }

        return IsLoaded();
    }
};
} // namespace aln