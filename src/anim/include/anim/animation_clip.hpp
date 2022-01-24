#pragma once

#include "bone_track.hpp"
#include "pose.hpp"

#include <assets/asset.hpp>

#include <vector>

namespace aln
{

/// @todo : Clarify difference between animation & animationClip
class AnimationClip : public IAsset
{
  public:
    /// @brief Sample the clip at a specific time
    /// @param time: Time to sample at
    /// @param pOutPose: Buffer to populate with the sampled pose
    void GetPose(float time, Pose* pOutPose) const
    {
        // TODO: Only sample tracks related to the pose's skeleton
        const auto pSkeleton = pOutPose->GetSkeleton();
        for (auto bone : pSkeleton->GetBones())
        {
            // TODO: how do we map bone->track ?
            //
            const auto& track = m_tracks[bone.handle];
            // auto transform = Sample(track, time); // TODO
            pOutPose->SetTransform(bone.handle, track.Sample(time));
        }
        // TODO
    }

  private:
    std::vector<Track> m_tracks;
    uint8_t m_frameRate; // Key frames per second
    // TODO
};
} // namespace aln