#pragma once

#include "bone.hpp"
#include "pose.hpp"
#include "skeleton.hpp"
#include "track.hpp"

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <common/types.hpp>

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
    AnimationClip(AssetID& id) : IAsset(id) {}

    /// @brief Sample the clip at a specific time
    /// @param time: Time to sample at
    /// @param pOutPose: Buffer to populate with the sampled pose
    void GetPose(float time, Pose* pOutPose) const
    {
        // TODO: Assert pose->skeleton == animClip->skeleton
        // TODO: Only sample tracks related to the pose's skeleton
        const auto pSkeleton = pOutPose->GetSkeleton();
        const auto boneCount = pSkeleton->GetBonesCount();
        for (BoneIndex i = 0; i < boneCount; ++i)
        {
            auto pBone = pSkeleton->GetBone(i);

            const auto boneIdx = pBone->GetIndex();
            const auto& track = m_tracks[boneIdx];

            assert(pBone->GetName() == track.GetBoneName());

            pOutPose->SetTransform(boneIdx, track.Sample(time));
        }
        // TODO
    }

    inline Seconds GetDuration() const { return m_duration; }
    inline size_t GetFrameCount() const { return m_tracks.size(); }
    inline uint32_t GetTicksPerSecond() const { return m_ticksPerSecond; }

  private:
    // Track components are in local bone space
    std::vector<Track> m_tracks;

    // TODO: Those are assimp values. Move them back to assimp converter and use Seconds/Frames here
    uint32_t m_ticksPerSecond = 0; // Ticks per second
    Seconds m_duration = 0.0f;     // Duration in ticks
};
} // namespace aln