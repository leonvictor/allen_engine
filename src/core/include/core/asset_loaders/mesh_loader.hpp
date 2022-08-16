#pragma once

#include <assets/asset_system/asset_system.hpp>
#include <assets/asset_system/mesh_asset.hpp>
#include <assets/loader.hpp>

#include "../mesh.hpp"

#include <memory>

namespace aln
{

class MeshLoader : public IAssetLoader
{
  private:
    std::shared_ptr<vkg::Device> m_pDevice;

  public:
    MeshLoader(std::shared_ptr<vkg::Device> pDevice) : m_pDevice(pDevice) {}

    bool Load(AssetRecord* pRecord, const assets::AssetFile& file) override
    {
        assert(pRecord->IsUnloaded());

        Mesh* pMesh = nullptr;

        if (pRecord->GetAssetTypeID() == SkeletalMesh::GetStaticAssetTypeID())
        {
            auto t1 = file.m_assetTypeID.ToString();
            auto t2 = SkeletalMesh::GetStaticAssetTypeID().ToString();
            assert(file.m_assetTypeID == SkeletalMesh::GetStaticAssetTypeID());

            SkeletalMesh* pSkeletalMesh = aln::New<SkeletalMesh>();

            auto info = assets::SkeletalMeshConverter::ReadInfo(&file);
            pSkeletalMesh->m_indices.resize(info.indexBufferSize / sizeof(uint32_t));
            pSkeletalMesh->m_vertices.resize(info.vertexBufferSize); // @note: vertex buffer is a byte vector
            pSkeletalMesh->m_inverseBindPose.resize(info.inverseBindPoseSize / sizeof(Transform));
            pSkeletalMesh->m_bindPose.resize(info.inverseBindPoseSize / sizeof(Transform));
            // pSkeletalMesh->m_pSkeleton = AssetHandle<Skeleton>(info.assetPath)

            // TODO: Inverse (DANGER)
            assets::SkeletalMeshConverter::Unpack(&info, file.m_binary, (std::byte*) pSkeletalMesh->m_vertices.data(), (std::byte*) pSkeletalMesh->m_indices.data(), (std::byte*) pSkeletalMesh->m_inverseBindPose.data());
            // assets::SkeletalMeshConverter::Unpack(&info, file.m_binary, (std::byte*) pMesh->m_vertices.data(), (std::byte*) pMesh->m_indices.data(), (std::byte*) pMesh->m_bindPose.data());

            for (size_t i = 0; i < pSkeletalMesh->m_bindPose.size(); ++i)
            {
                pSkeletalMesh->m_bindPose[i] = pSkeletalMesh->m_inverseBindPose[i].GetInverse();
                // pMesh->m_inverseBindPose[i] = pMesh->m_bindPose[i].GetInverse();
            }

            pMesh = pSkeletalMesh;
        }
        else
        {
            assert(file.m_assetTypeID == StaticMesh::GetStaticAssetTypeID());

            StaticMesh* pStaticMesh = aln::New<StaticMesh>();

            auto info = assets::StaticMeshConverter::ReadInfo(&file);
            pStaticMesh->m_indices.resize(info.indexBufferSize / sizeof(uint32_t));
            pStaticMesh->m_vertices.resize(info.vertexBufferSize);

            assets::StaticMeshConverter::Unpack(&info, file.m_binary, (std::byte*) pStaticMesh->m_vertices.data(), (std::byte*) pStaticMesh->m_indices.data());

            pMesh = pStaticMesh;
        }

        pMesh->CreateGraphicResources(m_pDevice);
        pRecord->SetAsset(pMesh);

        return true;
    }
};
} // namespace aln