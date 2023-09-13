#pragma once

#include <assets/asset.hpp>
#include <assets/loader.hpp>
#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>

#include "../texture.hpp"

#include <memory>

namespace aln
{

class TextureLoader : public IAssetLoader
{
  private:
    vkg::Device* m_pDevice;

  public:
    TextureLoader(vkg::Device* pDevice)
    {
        m_pDevice = pDevice;
    }

    bool Load(AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->GetAssetTypeID() == Texture::GetStaticAssetTypeID());

        Texture* pTex = aln::New<Texture>();

        int width, height;
        std::vector<std::byte> data;

        archive >> width;
        archive >> height;
        archive >> data;

        // TODO: only RGBA8 supported for now
        auto vkFormat = vk::Format::eR8G8B8A8Srgb;
        auto mipLevels = static_cast<uint32_t>(Maths::Floor(Maths::Log2(Maths::Max((float) width, (float) height)))) + 1;

        // Copy data to staging buffer
        // TODO: Skip the temporary buffer and stream directly to a vk::Buffer
        vkg::resources::Buffer stagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, data);
        data.clear();

        pTex->m_image = vkg::resources::Image::FromBuffer(m_pDevice, stagingBuffer, width, height, mipLevels, vkFormat);

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