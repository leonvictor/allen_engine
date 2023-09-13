#pragma once

#include "pose.hpp"
#include "skeleton.hpp"
#include "sync_track.hpp"
#include "track.hpp"

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <common/types.hpp>
#include <common/drawing_context.hpp>
#include <common/maths/maths.hpp>

#include <vector>

namespace aln
{

class AnimationEvent;

class FrameTime
{
  private:
    uint32_t m_frameIndex = 0;
    float m_percentageThroughFrame = 0.0f;

  public:
    FrameTime(uint32_t frameIndex, float percentageThroughFrame) : m_frameIndex(frameIndex), m_percentageThroughFrame(percentageThroughFrame) {}

    uint32_t GetFrameIndex() const { return m_frameIndex; }
    float GetPercentageThroughFrame() const { return m_percentageThroughFrame; }
    bool IsOnKeyFrame() const { return m_percentageThroughFrame == 0.0f || m_percentageThroughFrame == 1.0f; }
};

class AnimationClip : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("anim");

    friend class AnimationLoader;

  private:
    // Track components are in local bone space
    std::vector<Track> m_tracks;
    std::vector<Transform> m_rootMotionTrack;
    std::vector<AnimationEvent*> m_events;
    SyncTrack m_syncTrack;

    float m_framesPerSecond = 0.0f;
    uint32_t m_frameCount = 0;
    Seconds m_duration = 0.0f; // Duration in seconds

    FrameTime GetFrameTime(float percentageThroughAnimation) const
    {
        float frameIndex;
        float percentageThroughFrame = Maths::Modf(percentageThroughAnimation * (m_frameCount - 1), frameIndex);
        return FrameTime(frameIndex, percentageThroughFrame);
    }


    /// @brief Get the difference in root motion between two points in the animation (does not handle looping)
    inline Transform GetRootMotionDeltaNoLooping(float fromPercentageThroughAnimation, float toPercentageThroughAnimation) const
    {
        assert(fromPercentageThroughAnimation <= toPercentageThroughAnimation);

        const auto startRootMotion = GetRootMotion(GetFrameTime(fromPercentageThroughAnimation));
        const auto endRootMotion = GetRootMotion(GetFrameTime(toPercentageThroughAnimation));
        return Transform::Delta(startRootMotion, endRootMotion);
    }

  public:
    /// @brief Sample the clip at a specific time
    /// @param time: Time to sample at
    /// @param pOutPose: Buffer to populate with the sampled pose
    /// @todo Use frametime
      void GetPose(float time, Pose* pOutPose) const
    {
        assert(time <= m_duration);
        assert(pOutPose != nullptr);
        // TODO: Assert pose->skeleton == animClip->skeleton
        // TODO: Only sample tracks related to the pose's skeleton
        const auto pSkeleton = pOutPose->GetSkeleton();
        const auto boneCount = pSkeleton->GetBonesCount();

        float frameIndex;
        float frameProgress = Maths::Modf(time / GetFramesPerSecond(), frameIndex);

        for (BoneIndex boneIndex = 0; boneIndex < boneCount; ++boneIndex)
        {
            const auto& track = m_tracks[boneIndex];
            pOutPose->SetTransform(boneIndex, track.Sample((uint32_t) frameIndex, frameProgress));
        }
    }

    /// @brief Sample the root motion transform at a specific time
    Transform GetRootMotion(const FrameTime& frameTime) const
    {
        if (m_rootMotionTrack.empty())
        {
            return Transform::Identity;
        }

        if (frameTime.IsOnKeyFrame())
        {
            return m_rootMotionTrack[frameTime.GetFrameIndex()];
        }

        const auto& frameStart = m_rootMotionTrack[frameTime.GetFrameIndex()];
        const auto& frameEnd = m_rootMotionTrack[frameTime.GetFrameIndex() + 1];
        return Transform::Interpolate(frameStart, frameEnd, frameTime.GetPercentageThroughFrame());
    }

    /// @brief Get the difference in root motion between two points in the animation
    Transform GetRootMotionDelta(float fromPercentageThroughAnimation, float toPercentageThroughAnimation) const
    {
        if (fromPercentageThroughAnimation <= toPercentageThroughAnimation)
        {
            return GetRootMotionDeltaNoLooping(fromPercentageThroughAnimation, toPercentageThroughAnimation);
        }
        else // Handle looping
        {
            const auto startToLoopDelta = GetRootMotionDeltaNoLooping(fromPercentageThroughAnimation, 1.0f);
            const auto loopToEndDelta = GetRootMotionDeltaNoLooping(0.0f, toPercentageThroughAnimation);
            return loopToEndDelta * startToLoopDelta;
        }
    }

    inline Seconds GetDuration() const { return m_duration; }
    inline uint32_t GetFrameCount() const { return m_frameCount; }
    inline float GetFramesPerSecond() const { return m_framesPerSecond; }
    inline const SyncTrack& GetSyncTrack() const { return m_syncTrack; }

    void DrawRootMotionPath(DrawingContext& drawingContext, const Transform& worldTransform) const
    {
        assert(m_rootMotionTrack.size() > 0);
        
        auto transformCount = m_rootMotionTrack.size();
        for (auto transformIdx = 0; transformIdx < transformCount - 1; ++transformIdx)
        {
            const auto startTransform = worldTransform * m_rootMotionTrack[transformIdx];    
            const auto endTransform = worldTransform * m_rootMotionTrack[transformIdx + 1];    
            
            drawingContext.DrawLine(startTransform.GetTranslation(), endTransform.GetTranslation(), RGBColor::Blue);
        }
    }
};
} // namespace aln