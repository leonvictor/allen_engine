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
    Pose m_referencePose;

  public:
    Skeleton(AssetID& id) : IAsset(id), m_referencePose(this, Pose::InitialState::ReferencePose) {}

    inline size_t GetNumBones() const { return m_bones.size(); }
    inline const Bone* GetBone(BoneIndex index) const { return &m_bones[index]; }
    inline const Pose* GetReferencePose() const { return &m_referencePose; }

    /// @brief Get a flattened list of this skeleton's bones
    /// TODO: Use GetNumBones() + for loop instead
    // std::vector<const Bone*> GetBones() const
    // {
    //     // TODO
    //     return std::vector<const Bone*>();
    // }
};
} // namespace aln