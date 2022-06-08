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
    std::vector<BoneIndex> m_boneIndices;
    std::vector<Transform> m_bindPose;
    std::vector<Transform> m_inverseBindPose;

    // todo: move to component
    vkg::resources::Buffer m_skinningBuffer;
    std::vector<glm::mat4x4> m_skinningTransforms;

    /// @brief Create the vulkan buffers to back the mesh.
    virtual void CreateGraphicResources(const std::shared_ptr<vkg::Device>& pDevice) override
    {
        Mesh::CreateGraphicResources(pDevice);
        assert(!m_inverseBindPose.empty());
        // TODO: The buffer should be device-local
        m_skinningBuffer = vkg::resources::Buffer(pDevice, m_inverseBindPose.size() * sizeof(glm::mat4x4), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

  public:
    SkeletalMesh(AssetID& id) : Mesh(id) {}

    const std::vector<Transform>& GetBindPose() { return m_bindPose; }
    const std::vector<Transform>& GetInverseBindPose() { return m_inverseBindPose; }

    // TODO: Where should these be ?
    const vkg::resources::Buffer& GetSkinningBuffer() const { return m_skinningBuffer; }
};
} // namespace aln