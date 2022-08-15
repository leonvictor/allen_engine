#pragma once

#include <assets/asset.hpp>

#include "bone.hpp"
#include "pose.hpp"

#include <string>

namespace aln
{

/// TODO: On a mesh component, flag the skeleton that is to be used. Generate a mapping between the two. When setting a pose, transfer thoses bones that exist.
/// TODO: core bones (= modified by animators) vs deformation bones (= procedural, used by rendering, i.e. clothes). A skeleton can be made of both ?
/// @brief
class Skeleton : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("skel");

    friend class SkeletonLoader;

  private:
    Bone* m_rootBone = nullptr;
    std::vector<Bone> m_bones;
    /// @brief Default poses (bind for the rendering skeleton, reference for the anim one)
    std::vector<Transform> m_localReferencePose;
    std::vector<Transform> m_globalReferencePose;

  public:
    inline size_t GetBonesCount() const { return m_bones.size(); }
    inline const Bone* GetBone(BoneIndex index) const { return &m_bones[index]; }

    /// @brief Get the bone transforms in local (bone) space
    inline const std::vector<Transform>& GetLocalReferencePose() const { return m_localReferencePose; }

    /// @brief Get the bone transforms in global (character) space
    inline const std::vector<Transform>& GetGlobalReferencePose() const { return m_globalReferencePose; }
};
} // namespace aln