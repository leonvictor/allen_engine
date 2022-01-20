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
    std::string m_defaultMeshPath;

  public:
    MeshLoader(std::shared_ptr<vkg::Device> pDevice, std::string defaultMeshPath)
    {
        m_pDevice = pDevice;
        m_defaultMeshPath = defaultMeshPath;
    }

    bool Load(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsUnloaded());
        auto pMesh = AssetHandle<Mesh>(pAsset);

        assets::AssetFile file;
        auto loaded = assets::LoadBinaryFile(pMesh->GetID(), file);
        if (!loaded)
        {
            return false;
        }

        assert(file.type == assets::EAssetType::Mesh);

        auto info = assets::ReadMeshInfo(&file);
        pMesh->m_indices.resize(info.indexBufferSize);
        pMesh->m_vertices.resize(info.vertexBufferSize);
        assets::UnpackMesh(&info, file.binary, (std::byte*) pMesh->m_vertices.data(), (std::byte*) pMesh->m_indices.data());

        return true;
    }

    void Unload(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsLoaded());

        auto pMesh = AssetHandle<Mesh>(pAsset);
        pMesh->m_vertices.clear();
        pMesh->m_indices.clear();
        pMesh->m_primitives.clear();
    }

    void Initialize(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsLoaded());
        auto pMesh = AssetHandle<Mesh>(pAsset);
        pMesh->CreateGraphicResources(m_pDevice);
    }

    void Shutdown(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsInitialized());
        auto pMesh = AssetHandle<Mesh>(pAsset);
        pMesh->FreeGraphicResources();
    }
};

} // namespace aln