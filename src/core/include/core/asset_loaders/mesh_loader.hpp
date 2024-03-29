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
    RenderEngine* m_pRenderEngine;

    GPUBuffer m_vertexStagingBuffer;
    GPUBuffer m_indexStagingBuffer;

    static constexpr uint32_t STAGING_BUFFER_SIZE = 64 * 1000 * 1000 * 8;

  public:
    MeshLoader(RenderEngine* pDevice) : m_pRenderEngine(pDevice)
    {
    }

    ~MeshLoader()
    {
    }

    MeshLoader(const MeshLoader&) = delete;
    MeshLoader(MeshLoader&&) = delete;
    MeshLoader& operator=(const MeshLoader&) = delete;

    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override
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

        /// @todo GPU buffers could be all be kept in the renderer itself (in one large buffer that we index into)
        // Create and fill the vulkan buffers to back the mesh.
        pMesh->m_vertexBuffer.Initialize(m_pRenderEngine, pMesh->m_vertices.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        ctx.UploadBufferThroughStaging(pMesh->m_vertices, pMesh->m_vertexBuffer);
        
        pMesh->m_indexBuffer.Initialize(m_pRenderEngine, pMesh->m_indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        ctx.UploadBufferThroughStaging(pMesh->m_indices, pMesh->m_indexBuffer);

        pRecord->SetAsset(pMesh);

        return true;
    }

    void Unload(AssetRecord* pRecord) override
    {
        auto pMesh = pRecord->GetAsset<Mesh>();
        pMesh->m_indices.clear();
        pMesh->m_vertices.clear();
        pMesh->m_indexBuffer.Shutdown();
        pMesh->m_vertexBuffer.Shutdown();

        if (pRecord->GetAssetTypeID() == SkeletalMesh::GetStaticAssetTypeID())
        {
            auto pSkeletalMesh = pRecord->GetAsset<SkeletalMesh>();
            pSkeletalMesh->m_bindPose.clear();
            pSkeletalMesh->m_inverseBindPose.clear();
            pSkeletalMesh->m_boneNames.clear();
            pSkeletalMesh->m_parentBoneIndices.clear();
        }
    }

    void InstallDependencies(AssetRecord* pAssetRecord, const Vector<IAssetHandle>& dependencies) override
    {
        assert(dependencies.size() == 1);
        auto pMesh = pAssetRecord->GetAsset<Mesh>();

        const auto pMaterialRecord = GetDependencyRecord(dependencies, 0);
        UpdateDependencyRecord(pMesh->m_pMaterial, pMaterialRecord);

        pMesh->m_descriptorSet = m_pRenderEngine->AllocateDescriptorSet<Mesh>();
        auto textureDescriptor = pMesh->m_pMaterial->GetAlbedoMap()->GetDescriptor();
        auto materialDescriptor = pMesh->GetMaterial()->GetBuffer().GetDescriptor();

        Vector<vk::WriteDescriptorSet> writeDescriptors = {
            {
                .dstSet = pMesh->m_descriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &textureDescriptor,
            },
            {
                .dstSet = pMesh->m_descriptorSet,
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &materialDescriptor,
            },
        };

        m_pRenderEngine->GetVkDevice().updateDescriptorSets(writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

        m_pRenderEngine->SetDebugUtilsObjectName(pMesh->m_descriptorSet, "Mesh Descriptor Set");
    }
};
} // namespace aln