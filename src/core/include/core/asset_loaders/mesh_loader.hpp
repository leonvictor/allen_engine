#pragma once

#include <assets/loader.hpp>

#include "../mesh.hpp"

#include <memory>

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
            archive >> pSkeletalMesh->m_inverseBindPose;

            // Generate bind pose
            // TODO: Are both really needed ?
            pSkeletalMesh->m_bindPose.reserve(pSkeletalMesh->m_inverseBindPose.size());
            for (size_t i = 0; i < pSkeletalMesh->m_inverseBindPose.size(); ++i)
            {
                pSkeletalMesh->m_bindPose.push_back(pSkeletalMesh->m_inverseBindPose[i].GetInverse());
            }

            // pSkeletalMesh->m_pSkeleton = AssetHandle<Skeleton>(info.assetPath)

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

        // Create and fill the vulkan buffers to back the mesh.
        // Create vertex buffer
        vkg::resources::Buffer vertexStagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, pMesh->m_vertices);
        pMesh->m_vertexBuffer = vkg::resources::Buffer(m_pDevice, vertexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        // Create index buffer
        vkg::resources::Buffer indexStagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, pMesh->m_indices);
        pMesh->m_indexBuffer = vkg::resources::Buffer(m_pDevice, indexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        m_pDevice->GetTransferCommandPool().Execute([&](vk::CommandBuffer cb)
            {
            vertexStagingBuffer.CopyTo(cb, pMesh->m_vertexBuffer);
            indexStagingBuffer.CopyTo(cb, pMesh->m_indexBuffer); });

        pRecord->SetAsset(pMesh);

        return true;
    }
};
} // namespace aln