#pragma once

#include <anim/animation_clip.hpp>
#include <anim/pose.hpp>
#include <anim/skeleton.hpp>
#include <assets/asset_service.hpp>
#include <assets/handle.hpp>
#include <common/types.hpp>
#include <entities/component.hpp>

namespace aln
{

class AnimationPlayerComponent : public IComponent
{
    ALN_REGISTER_TYPE();

  private:
    AssetHandle<AnimationClip> m_pAnimationClip;

    Pose* m_pPose = nullptr;
    Percentage m_previousAnimTime = 0.0f;
    Percentage m_animTime = 0.0f;

    // Editor
    // TODO: Move out / Pause the whole world rather than just an anim player
    bool m_pause = false;

  public:
    inline const Pose* GetPose()
    {
        assert(m_pPose != nullptr);
        return m_pPose;
    }

    inline const AnimationClip* GetAnimationClip() const { return m_pAnimationClip.get(); }

    void SetAnimationClip(const AssetID& animationClipID)
    {
        assert(IsUnloaded());
        m_pAnimationClip = AssetHandle<AnimationClip>(animationClipID);
    }

    void Update(Seconds deltaTime);

    void Initialize() override
    {
        assert(m_pAnimationClip.IsLoaded());
        m_pPose = aln::New<Pose>(m_pAnimationClip->GetSkeleton());
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