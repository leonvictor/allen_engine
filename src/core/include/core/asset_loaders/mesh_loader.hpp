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

    resources::Buffer m_vertexStagingBuffer;
    resources::Buffer m_indexStagingBuffer;

    static constexpr uint32_t STAGING_BUFFER_SIZE = 64 * 1000 * 1000 * 8;

  public:
    MeshLoader(RenderEngine* pDevice) : m_pRenderEngine(pDevice)
    {
        m_vertexStagingBuffer = resources::Buffer(
            m_pRenderEngine,
            STAGING_BUFFER_SIZE,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        
        m_pRenderEngine->SetDebugUtilsObjectName(m_vertexStagingBuffer.GetVkBuffer(), "Mesh Loader Vertex Staging Buffer");

        m_indexStagingBuffer = resources::Buffer(
            m_pRenderEngine,
            STAGING_BUFFER_SIZE,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        m_pRenderEngine->SetDebugUtilsObjectName(m_indexStagingBuffer.GetVkBuffer(), "Mesh Loader Index Staging Buffer");

        m_vertexStagingBuffer.Map();
        m_indexStagingBuffer.Map();
    }

    ~MeshLoader()
    {
        m_vertexStagingBuffer.Shutdown();
        m_indexStagingBuffer.Shutdown();
    }

    MeshLoader(const MeshLoader&) = delete;
    MeshLoader(MeshLoader&&) = delete;
    MeshLoader& operator=(const MeshLoader&) = delete;

    bool Load(RequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override
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
        m_vertexStagingBuffer.Copy(pMesh->m_vertices);
        pMesh->m_vertexBuffer = resources::Buffer(m_pRenderEngine, pMesh->m_vertices.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_pRenderEngine->SetDebugUtilsObjectName(pMesh->m_vertexBuffer.GetVkBuffer(), "Mesh Vertex Buffer");

        m_indexStagingBuffer.Copy(pMesh->m_indices);
        pMesh->m_indexBuffer = resources::Buffer(m_pRenderEngine, pMesh->m_indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_pRenderEngine->SetDebugUtilsObjectName(pMesh->m_indexBuffer.GetVkBuffer(), "Mesh Index Buffer");

        // TODO: Only record command buffers here
        // This method is called from multiple thread and we must submit them at once
        auto pCB = ctx.GetTransferCommandBuffer();
        m_vertexStagingBuffer.CopyTo(*pCB, pMesh->m_vertexBuffer);
        m_indexStagingBuffer.CopyTo(*pCB, pMesh->m_indexBuffer);

        pRecord->SetAsset(pMesh);

        return true;
    }

    void Unload(AssetRecord* pRecord) override
    {
        auto pMesh = pRecord->GetAsset<Mesh>();
        pMesh->m_indices.clear();
        pMesh->m_vertices.clear();
        pMesh->m_descriptorSet.reset();
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

        auto pMaterialRecord = GetDependencyRecord(dependencies, 0);
        pMesh->m_pMaterial.m_pAssetRecord = pMaterialRecord;

        auto descriptorSet = m_pRenderEngine->AllocateDescriptorSet<Mesh>();

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

        m_pRenderEngine->GetVkDevice().updateDescriptorSets(writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

        pMesh->m_descriptorSet = std::move(descriptorSet);
        m_pRenderEngine->SetDebugUtilsObjectName(pMesh->m_descriptorSet.get(), "Mesh Descriptor Set");
    }
};
} // namespace aln