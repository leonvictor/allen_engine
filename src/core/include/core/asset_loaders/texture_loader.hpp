#pragma once

#include <assets/asset.hpp>
#include <assets/asset_system/texture_asset.hpp>
#include <assets/loader.hpp>

#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>

#include "../texture.hpp"

#include <memory>

namespace aln
{

vk::Format MapAssetFormatToVulkan(assets::TextureFormat& format)
{
    switch (format)
    {
    case assets::TextureFormat::RGBA8:
        return vk::Format::eR8G8B8A8Srgb;
    case assets::TextureFormat::RGB8:
        return vk::Format::eR8G8B8Srgb;
    default:
        throw; // TODO
    }
}

class TextureLoader : public IAssetLoader
{
  private:
    vkg::Device* m_pDevice;

  public:
    TextureLoader(vkg::Device* pDevice)
    {
        m_pDevice = pDevice;
    }

    bool Load(AssetRecord* pRecord, const assets::AssetFile& file) override
    {
        assert(pRecord->IsUnloaded());
        assert(file.m_assetTypeID == Texture::GetStaticAssetTypeID());

        Texture* pTex = aln::New<Texture>();

        auto info = assets::ReadTextureInfo(&file);
        auto format = MapAssetFormatToVulkan(info.format);
        auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(info.pixelSize[0], info.pixelSize[1])))) + 1;

        std::vector<std::byte> data;
        assets::UnpackTexture(&info, file.m_binary, data);

        // Copy data to staging buffer
        // TODO: Skip the temporary buffer and stream directly to a vk::Buffer
        vkg::resources::Buffer stagingBuffer(m_pDevice, (vk::DeviceSize) info.size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, data.data());
        data.clear();

        pTex->m_image = vkg::resources::Image::FromBuffer(m_pDevice, stagingBuffer, info.pixelSize[0], info.pixelSize[1], mipLevels, format);

        // TODO: Abstract io to another lib
        // TODO: Split image loading and vulkan texture creation
        // TODO: Handle 2D/3D Textures here

        // Vulkan objects loading happens during initialization as we do not handle threaded creation yet
        pTex->m_image.AddView();
        pTex->m_image.AddSampler();
        pTex->m_image.SetDebugName("Material Texture"); // todo: add id name

        pRecord->SetAsset(pTex);
        return true;
    }
};

} // namespace aln