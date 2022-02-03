#pragma once

#include "mesh.hpp"

#include <anim/bone.hpp>
#include <common/transform.hpp>

namespace aln
{
class SkeletalMesh : public Mesh
{
  private:
    // TODO: SkeletalMesh uses a different Vertex format

    std::vector<BoneIndex> m_boneIndices;
    std::vector<Transform> m_bindPose;
    std::vector<Transform> m_inverseBindPose;

  public:
    const std::vector<Transform>& GetBindPose() { return m_bindPose; }
    const std::vector<Transform>& GetInverseBindPose() { return m_inverseBindPose; }
};
} // namespace aln