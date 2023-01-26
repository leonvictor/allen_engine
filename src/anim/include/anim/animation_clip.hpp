#pragma once

#include "pose.hpp"
#include "skeleton.hpp"
#include "track.hpp"

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <common/types.hpp>

#include <cmath>
#include <iostream>
#include <vector>

namespace aln
{

/// @todo : Clarify difference between animation & animationClip
class AnimationClip : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("anim");

    friend class AnimationLoader;

  public:
    /// @brief Sample the clip at a specific time
    /// @param time: Time to sample at
    /// @param pOutPose: Buffer to populate with the sampled pose
    void GetPose(float time, Pose* pOutPose) const
    {
        assert(time <= m_duration);
        // TODO: Assert pose->skeleton == animClip->skeleton
        // TODO: Only sample tracks related to the pose's skeleton
        const auto pSkeleton = pOutPose->GetSkeleton();
        const auto boneCount = pSkeleton->GetBonesCount();

        float frameIndex;
        float frameProgress = std::modf(time / GetFramesPerSecond(), &frameIndex);

        for (BoneIndex boneIndex = 0; boneIndex < boneCount; ++boneIndex)
        {
            const auto& track = m_tracks[boneIndex];
            pOutPose->SetTransform(boneIndex, track.Sample((uint32_t) frameIndex, frameProgress));
        }
    }

    inline Seconds GetDuration() const { return m_duration; }
    inline size_t GetFrameCount() const { return m_tracks.size(); }
    inline float GetFramesPerSecond() const { return m_framesPerSecond; }

  private:
    // Track components are in local bone space
    std::vector<Track> m_tracks;

    float m_framesPerSecond = 0.0f;
    Seconds m_duration = 0.0f; // Duration in seconds
};
} // namespace aln