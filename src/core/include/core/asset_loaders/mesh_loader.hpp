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

        // TODO: ?
        pMesh->Load(pMesh->GetID());
        pMesh->CreateGraphicResources(m_pDevice);

        return true;
    }

    void Unload(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsLoaded());
        auto pMesh = AssetHandle<Mesh>(pAsset);
        pMesh->FreeGraphicResources();
        pMesh->Unload();
    }
};

} // namespace aln