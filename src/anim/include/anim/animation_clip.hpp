#pragma once

#include "frame_time.hpp"
#include "pose.hpp"
#include "skeleton.hpp"
#include "sync_track.hpp"
#include "track.hpp"

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <common/containers/vector.hpp>
#include <common/drawing_context.hpp>
#include <common/maths/maths.hpp>
#include <common/types.hpp>

namespace aln
{

class AnimationEvent;

class AnimationClip : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("anim");

    friend class AnimationLoader;

  private:
    // Track components are in local bone space
    Vector<Track> m_tracks;
    Vector<Transform> m_rootMotionTrack;
    Vector<AnimationEvent*> m_events;
    SyncTrack m_syncTrack;

    float m_framesPerSecond = 0.0f;
    uint32_t m_frameCount = 0;
    Seconds m_duration = 0.0f; // Duration in seconds

    AssetHandle<Skeleton> m_pSkeleton;

    /// @brief Get the difference in root motion between two points in the animation (does not handle looping)
    inline Transform GetRootMotionDeltaNoLooping(Percentage fromPercentageThroughAnimation, Percentage toPercentageThroughAnimation) const
    {
        assert(fromPercentageThroughAnimation <= toPercentageThroughAnimation);

        const auto startRootMotion = GetRootMotion(GetFrameTime(fromPercentageThroughAnimation));
        const auto endRootMotion = GetRootMotion(GetFrameTime(toPercentageThroughAnimation));
        return Transform::Delta(startRootMotion, endRootMotion);
    }

  public:
    const Skeleton* GetSkeleton() const { return m_pSkeleton.get(); }

    /// @brief Sample the clip at a specific time
    void GetPose(const FrameTime& frameTime, Pose* pOutPose) const
    {
        assert(frameTime.GetFrameIndex() < GetFrameCount() && pOutPose != nullptr);
        assert(pOutPose->GetSkeleton() == m_pSkeleton.get());

        // TODO: Only sample tracks related to the pose's skeleton
        const auto pSkeleton = pOutPose->GetSkeleton();
        const auto boneCount = pSkeleton->GetBonesCount();

        for (BoneIndex boneIndex = 0; boneIndex < boneCount; ++boneIndex)
        {
            const auto& track = m_tracks[boneIndex];
            pOutPose->SetTransform(boneIndex, track.Sample(frameTime));
        }
    }

    /// @brief Sample the clip at a specific percentage through
    /// @param pOutPose: Buffer to populate with the sampled pose
    void GetPose(Percentage percentageThroughAnimation, Pose* pOutPose) const { GetPose(GetFrameTime(percentageThroughAnimation), pOutPose); }

    FrameTime GetFrameTime(Percentage percentageThroughAnimation) const
    {
        float frameIndex;
        Percentage percentageThroughFrame = Maths::Modf(percentageThroughAnimation * (m_frameCount - 1), frameIndex);
        return FrameTime(frameIndex, percentageThroughFrame);
    }

    Percentage GetPercentageThrough(const FrameTime& frameTime) const { return ((float) frameTime) * (1.0f / (m_frameCount - 1)); }

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
    Transform GetRootMotionDelta(Percentage fromPercentageThroughAnimation, Percentage toPercentageThroughAnimation) const
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

    /// @todo Editor only / Move to an editor class ? Maybe Anim Clip Workspace ? We could use it in importer as well
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