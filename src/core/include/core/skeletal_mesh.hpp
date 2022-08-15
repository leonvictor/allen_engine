#pragma once

#include "mesh.hpp"

#include <anim/bone.hpp>
#include <common/transform.hpp>

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

    /// @brief Create the vulkan buffers to back the mesh.
    virtual void CreateGraphicResources(const std::shared_ptr<vkg::Device>& pDevice) override
    {
        Mesh::CreateGraphicResources(pDevice);
        assert(!m_inverseBindPose.empty());
    }

  public:
    const std::vector<Transform>& GetBindPose() const { return m_bindPose; }
    const std::vector<Transform>& GetInverseBindPose() const { return m_inverseBindPose; }
};
} // namespace aln