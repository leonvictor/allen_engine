#pragma once

#include "pose.hpp"

#include <assets/asset.hpp>

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
    // TODO: Use StringID
    std::vector<std::string> m_boneNames;
    std::vector<BoneIndex> m_parentBoneIndices;

    /// @brief Default poses (bind for the rendering skeleton, reference for the anim one)
    std::vector<Transform> m_localReferencePose;
    std::vector<Transform> m_globalReferencePose;

  public:
    inline size_t GetBonesCount() const { return m_boneNames.size(); }
    inline const std::string& GetBoneName(BoneIndex boneIndex) const { return m_boneNames[boneIndex]; }
    inline BoneIndex GetParentBoneIndex(BoneIndex boneIndex) const { return m_parentBoneIndices[boneIndex]; }

    /// @brief Get the bone transforms in local (bone) space
    inline const std::vector<Transform>& GetLocalReferencePose() const { return m_localReferencePose; }

    /// @brief Get the bone transforms in global (character) space
    inline const std::vector<Transform>& GetGlobalReferencePose() const { return m_globalReferencePose; }
};
} // namespace aln