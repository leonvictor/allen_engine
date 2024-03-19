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

    friend class AnimationSystem;

  private:
    AssetHandle<AnimationClip> m_pAnimationClip;

    Pose* m_pPose = nullptr;
    Percentage m_previousPercentageThroughAnimation = 0.0f; // Previous percentage through the animation
    Percentage m_percentageThroughAnimation = 0.0f;         // Percentage through the animation

    // Editor
    bool m_pause = false;

  private:
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

    inline void SetPaused(bool paused) { m_pause = paused; }
    inline bool IsPaused() const { return m_pause; }
    
    inline void SetPercentageThroughAnimation(Percentage percentageThroughAnimation)
    {
        m_previousPercentageThroughAnimation = m_percentageThroughAnimation;
        m_percentageThroughAnimation = percentageThroughAnimation;
    }
    inline Percentage GetPercentageThroughAnimation() const { return m_percentageThroughAnimation; }

    inline FrameTime GetFrameTime() const { return m_pAnimationClip->GetFrameTime(m_percentageThroughAnimation); }
    void SetFrameTime(const FrameTime& frameTime)
    {
        assert(m_pAnimationClip.IsValid());

        const auto percentageAtFrameTime = m_pAnimationClip->GetPercentageThrough(frameTime);
        SetPercentageThroughAnimation(percentageAtFrameTime);
    }
};
} // namespace aln