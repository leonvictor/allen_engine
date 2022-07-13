#pragma once

#include <assets/asset.hpp>
#include <assets/loader.hpp>

#include "../material.hpp"

#include <memory>

namespace aln
{

class MaterialLoader : public IAssetLoader<Material>
{
  private:
    std::shared_ptr<vkg::Device> m_pDevice;

  public:
    MaterialLoader(std::shared_ptr<vkg::Device> pDevice)
    {
        m_pDevice = pDevice;
    }

    bool Load(AssetRecord* pRecord) override
    {
        auto pMat = pRecord->GetAsset<Material>();
        pMat->m_buffer = vkg::resources::Buffer(m_pDevice, sizeof(MaterialBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

        // TMP while materials are poopy
        auto material = MaterialBufferObject();
        pMat->m_buffer.Map(0, sizeof(material));
        pMat->m_buffer.Copy(&material, sizeof(material));
        pMat->m_buffer.Unmap();

        return true;
    }

    void Unload(AssetRecord* pRecord) override
    {
        auto pMat = pRecord->GetAsset<Material>();
        pMat->m_buffer = vkg::resources::Buffer();
    }
};

} // namespace aln