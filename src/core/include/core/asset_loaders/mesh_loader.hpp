#pragma once

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
        // TODO: Allow various loading modes...
        return pMesh->Load(pMesh->GetID());
    }

    void Unload(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsLoaded());
        auto pMesh = AssetHandle<Mesh>(pAsset);
        pMesh->Unload();
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