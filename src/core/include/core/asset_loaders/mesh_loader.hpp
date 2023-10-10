#pragma once

#include "../mesh.hpp"
#include "../skeletal_mesh.hpp"
#include "../static_mesh.hpp"

#include <assets/loader.hpp>

namespace aln
{

class MeshLoader : public IAssetLoader
{
  private:
    vkg::Device* m_pDevice;

  public:
    MeshLoader(vkg::Device* pDevice) : m_pDevice(pDevice) {}

    bool Load(AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());

        Mesh* pMesh = nullptr;

        if (pRecord->GetAssetTypeID() == SkeletalMesh::GetStaticAssetTypeID())
        {
            SkeletalMesh* pSkeletalMesh = aln::New<SkeletalMesh>();

            archive >> pSkeletalMesh->m_indices;
            archive >> pSkeletalMesh->m_vertices;
            archive >> pSkeletalMesh->m_boneNames;
            archive >> pSkeletalMesh->m_parentBoneIndices;
            archive >> pSkeletalMesh->m_inverseBindPose;

            // Generate bind pose
            const auto boneCount = pSkeletalMesh->m_inverseBindPose.size();
            pSkeletalMesh->m_bindPose.reserve(boneCount);
            for (size_t i = 0; i < boneCount; ++i)
            {
                pSkeletalMesh->m_bindPose.push_back(pSkeletalMesh->m_inverseBindPose[i].GetInverse());
            }

            pMesh = pSkeletalMesh;
        }
        else
        {
            assert(pRecord->GetAssetTypeID() == StaticMesh::GetStaticAssetTypeID());

            StaticMesh* pStaticMesh = aln::New<StaticMesh>();

            archive >> pStaticMesh->m_indices;
            archive >> pStaticMesh->m_vertices;

            pMesh = pStaticMesh;
        }

        assert(!pMesh->m_indices.empty() && !pMesh->m_vertices.empty());

        /// @todo GPU buffers could be all be kept in the renderer itself
        // Create and fill the vulkan buffers to back the mesh.
        vkg::resources::Buffer vertexStagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, pMesh->m_vertices);
        pMesh->m_vertexBuffer = vkg::resources::Buffer(m_pDevice, vertexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        vkg::resources::Buffer indexStagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, pMesh->m_indices);
        pMesh->m_indexBuffer = vkg::resources::Buffer(m_pDevice, indexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        m_pDevice->GetTransferCommandPool().Execute([&](vk::CommandBuffer cb)
            {
            vertexStagingBuffer.CopyTo(cb, pMesh->m_vertexBuffer);
            indexStagingBuffer.CopyTo(cb, pMesh->m_indexBuffer); });

        pRecord->SetAsset(pMesh);

        return true;
    }

    void InstallDependencies(AssetRecord* pAssetRecord, const Vector<IAssetHandle>& dependencies) override
    {
        assert(dependencies.size() == 1);
        auto pMesh = pAssetRecord->GetAsset<Mesh>();

        auto pMaterialRecord = GetDependencyRecord(dependencies, 0);
        pMesh->m_pMaterial.m_pAssetRecord = pMaterialRecord;

        auto descriptorSet = m_pDevice->AllocateDescriptorSet<Mesh>();

        auto textureDescriptor = pMesh->m_pMaterial->GetAlbedoMap()->GetDescriptor();
        auto materialDescriptor = pMesh->GetMaterial()->GetBuffer().GetDescriptor();

        Vector<vk::WriteDescriptorSet> writeDescriptors = {
            {
                .dstSet = descriptorSet.get(),
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &textureDescriptor,
            },
            {
                .dstSet = descriptorSet.get(),
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &materialDescriptor,
            },
        };

        m_pDevice->GetVkDevice().updateDescriptorSets(writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

        pMesh->m_descriptorSet = std::move(descriptorSet);
        m_pDevice->SetDebugUtilsObjectName(pMesh->m_descriptorSet.get(), "Mesh Descriptor Set");
    }
};
} // namespace aln