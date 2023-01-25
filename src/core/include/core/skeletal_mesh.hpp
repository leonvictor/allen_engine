#pragma once

#include "mesh.hpp"

#include <common/transform.hpp>
#include <reflection/type_info.hpp>

namespace aln
{
class SkeletalMesh : public Mesh
{
    friend class MeshLoader;
    friend class SkeletalMeshComponent;

    ALN_REGISTER_ASSET_TYPE("smsh")

  private:
    // Bind pose in global space
    std::vector<Transform> m_bindPose;        // Mesh space -> Bone space
    std::vector<Transform> m_inverseBindPose; // Bone space -> Mesh space

  public:
    const std::vector<Transform>& GetBindPose() const { return m_bindPose; }
    const std::vector<Transform>& GetInverseBindPose() const { return m_inverseBindPose; }
    size_t GetBoneCount() const { return m_bindPose.size(); }
};

ALN_REGISTER_PRIMITIVE(SkeletalMesh);
} // namespace aln