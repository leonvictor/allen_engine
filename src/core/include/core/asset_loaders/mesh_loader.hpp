#pragma once

#include <assets/asset_system/asset_system.hpp>
#include <assets/asset_system/mesh_asset.hpp>
#include <assets/loader.hpp>

#include "../mesh.hpp"

#include <memory>

namespace aln
{

class MeshLoader : public IAssetLoader<Mesh>
{
  private:
    std::shared_ptr<vkg::Device> m_pDevice;

  public:
    MeshLoader(std::shared_ptr<vkg::Device> pDevice) : m_pDevice(pDevice) {}

    // AssetHandle<IAsset> Create(AssetID id)
    // {
    //     if (id.GetAssetTypeID() == SkeletalMesh::GetStaticAssetTypeID())
    //     {
    //         return AssetHandle<IAsset>(std::make_shared<SkeletalMesh>(id));
    //     }
    //     else
    //     {
    //         assert(id.GetAssetTypeID() == StaticMesh::GetStaticAssetTypeID());
    //         return AssetHandle<IAsset>(std::make_shared<StaticMesh>(id));
    //     }
    // }

    bool Load(AssetRecord* pRecord) override
    {
        auto pAsset = pRecord->GetAsset();
        assert(pAsset->IsUnloaded());

        assets::AssetFile file;
        auto loaded = assets::LoadBinaryFile(pRecord->GetID().GetAssetPath(), file);
        if (!loaded)
        {
            return false;
        }

        bool Load(AssetRecord * pRecord) override
        {
            assert(file.type == assets::EAssetType::SkeletalMesh);
            auto pMesh = pRecord->GetAsset<SkeletalMesh>();

            auto info = assets::SkeletalMeshConverter::ReadInfo(&file);
            pMesh->m_indices.resize(info.indexBufferSize / sizeof(uint32_t));
            pMesh->m_vertices.resize(info.vertexBufferSize); // @note: vertex buffer is a byte vector
            pMesh->m_inverseBindPose.resize(info.inverseBindPoseSize / sizeof(Transform));
            pMesh->m_bindPose.resize(info.inverseBindPoseSize / sizeof(Transform));

            // TODO: Inverse (DANGER)
            assets::SkeletalMeshConverter::Unpack(&info, file.binary, (std::byte*) pMesh->m_vertices.data(), (std::byte*) pMesh->m_indices.data(), (std::byte*) pMesh->m_inverseBindPose.data());
            // assets::SkeletalMeshConverter::Unpack(&info, file.binary, (std::byte*) pMesh->m_vertices.data(), (std::byte*) pMesh->m_indices.data(), (std::byte*) pMesh->m_bindPose.data());

            for (size_t i = 0; i < pMesh->m_bindPose.size(); ++i)
            {
                pMesh->m_bindPose[i] = pMesh->m_inverseBindPose[i].GetInverse();
                // pMesh->m_inverseBindPose[i] = pMesh->m_bindPose[i].GetInverse();
            }
            pMesh->CreateGraphicResources(m_pDevice);
        }
        else
        {
            assert(file.type == assets::EAssetType::StaticMesh);
            auto pMesh = pRecord->GetAsset<StaticMesh>();

            auto info = assets::StaticMeshConverter::ReadInfo(&file);
            pMesh->m_indices.resize(info.indexBufferSize / sizeof(uint32_t));
            pMesh->m_vertices.resize(info.vertexBufferSize);

            assets::StaticMeshConverter::Unpack(&info, file.binary, (std::byte*) pMesh->m_vertices.data(), (std::byte*) pMesh->m_indices.data());
            pMesh->CreateGraphicResources(m_pDevice);
        }

        return true;
    }

    void Unload(AssetRecord* pRecord) override
    {
        auto pAsset = pRecord->GetAsset();
        assert(pAsset->IsLoaded());

        auto pMesh = pRecord->GetAsset<StaticMesh>();
        pMesh->FreeGraphicResources();
        pMesh->m_vertices.clear();
        pMesh->m_indices.clear();
        pMesh->m_primitives.clear();

        // TODO: bind pose as well
    }
};

} // namespace aln