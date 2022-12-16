#pragma once

#include <assets/asset.hpp>
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

    bool Load(AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->GetAssetTypeID() == Material::GetStaticAssetTypeID());

        Material* pMaterial = aln::New<Material>();

        // TODO: Extract directly to asset handles
        AssetID id;
        archive >> id;
        pMaterial->m_albedoMap = AssetHandle<Texture>(id);

        // TMP while materials are poopy
        pMaterial->m_buffer = vkg::resources::Buffer(m_pDevice, sizeof(MaterialBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

        auto material = MaterialBufferObject();
        pMaterial->m_buffer.Map(0, sizeof(material));
        pMaterial->m_buffer.Copy(&material, sizeof(material));
        pMaterial->m_buffer.Unmap();

        pRecord->SetAsset(pMaterial);

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