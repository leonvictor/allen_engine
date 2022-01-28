#pragma once

#include <assets/asset.hpp>

namespace aln
{

struct Bone
{
    uint16_t m_handle;
    std::string m_name;

    Bone* pParent;
    std::vector<Bone> children;
};

/// TODO: On a mesh component, flag the skeleton that is to be used. Generate a mapping between the two. When setting a pose, transfer thoses bones that exist.
/// TODO: core bones (= modified by animators) vs deformation bones (= procedural, used by rendering, i.e. clothes). A skeleton can be made of both ?
/// @brief
class Skeleton : public IAsset
{
  private:
    Bone m_rootBone;
    // TODO: Default poses (bind for the rendering skeleton, reference for the anim one)

  public:
    uint32_t GetNumBones() const
    {
        // TODO
        return 0;
    }

    /// @brief Get a flattened list of this skeleton's bones
    std::vector<const Bone*> GetBones() const
    {
        // TODO
        return std::vector<const Bone*>();
    }
};
} // namespace aln