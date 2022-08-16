#pragma once

#include <assets/asset.hpp>
#include <assets/asset_system/material_asset.hpp>
#include <assets/loader.hpp>

#include "../material.hpp"

#include <memory>

namespace aln
{

class MaterialLoader : public IAssetLoader
{
  private:
    vkg::Device* m_pDevice;

  public:
    MaterialLoader(vkg::Device* pDevice)
    {
        m_pDevice = pDevice;
    }

    bool Load(AssetRecord* pRecord, const assets::AssetFile& file) override
    {
        assert(pRecord->IsUnloaded());
        assert(file.m_assetTypeID == Material::GetStaticAssetTypeID());

        Material* pMat = aln::New<Material>();

        auto info = assets::ReadMaterialInfo(&file);
        pMat->m_albedoMap = AssetHandle<Texture>(info.m_albedoMapID);

        // TMP while materials are poopy
        pMat->m_buffer = vkg::resources::Buffer(m_pDevice, sizeof(MaterialBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

        auto material = MaterialBufferObject();
        pMat->m_buffer.Map(0, sizeof(material));
        pMat->m_buffer.Copy(&material, sizeof(material));
        pMat->m_buffer.Unmap();

        pRecord->SetAsset(pMat);

        return true;
    }

    void InstallDependencies(AssetRecord* pAssetRecord, const std::vector<IAssetHandle>& dependencies) override
    {
        assert(dependencies.size() == 1);
        auto pMaterial = pAssetRecord->GetAsset<Material>();

        auto pAlbedoMapRecord = GetDependencyRecord(dependencies, pMaterial->m_albedoMap.GetAssetID());
        pMaterial->m_albedoMap.m_pAssetRecord = pAlbedoMapRecord;
    }
};

} // namespace aln