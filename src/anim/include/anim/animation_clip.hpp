#pragma once

#include "bone.hpp"
#include "pose.hpp"
#include "skeleton.hpp"
#include "track.hpp"

#include <assets/asset.hpp>

#include <vector>

namespace aln
{

/// @todo : Clarify difference between animation & animationClip
class AnimationClip : public IAsset
{
    friend class AnimationLoader;

  public:
    AnimationClip(AssetGUID& guid) : IAsset(guid) {}

    /// @brief Sample the clip at a specific time
    /// @param time: Time to sample at
    /// @param pOutPose: Buffer to populate with the sampled pose
    void GetPose(float time, Pose* pOutPose) const
    {
        // TODO: Only sample tracks related to the pose's skeleton
        const auto pSkeleton = pOutPose->GetSkeleton();
        const auto size = pSkeleton->GetNumBones();
        for (size_t i = 0; i < size; ++i)
        {
            auto pBone = pSkeleton->GetBone(i);
            // TODO: how do we map bone->track ?
            //
            const auto boneIdx = pBone->GetIndex();
            const auto& track = m_tracks[boneIdx];
            pOutPose->SetTransform(boneIdx, track.Sample(time));
        }
        // TODO
    }

  private:
    std::vector<Track> m_tracks;
    uint8_t m_frameRate; // Key frames per second
    // TODO
};
} // namespace aln