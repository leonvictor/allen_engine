#pragma once

#include <assets/asset.hpp>
#include <assets/loader.hpp>
#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>

#include "../texture.hpp"

namespace aln
{

class TextureLoader : public IAssetLoader
{
  private:
    RenderEngine* m_pRenderEngine;

    GPUBuffer m_stagingBuffer;

  public:
    TextureLoader(RenderEngine* pDevice)
    {
        m_pRenderEngine = pDevice;

        // TODO: Use a smarter allocation strategy for staging buffers
        static const uint32_t stagingBufferSize = 64 * 1000 * 1000 * 8;
        m_stagingBuffer.Initialize(
            m_pRenderEngine,
            stagingBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        m_pRenderEngine->SetDebugUtilsObjectName(m_stagingBuffer.GetVkBuffer(), "Texture Loader Staging Buffer");

        m_stagingBuffer.Map();

        // Strategy : Fixed sized buffer. Keep an offset of what has already been reserved for a transfer. Reset that index when a command finishes
        // If there is not enough memory, wait the next cycle
    }

    ~TextureLoader()
    {
        m_stagingBuffer.Unmap();
        m_stagingBuffer.Shutdown();
    }

    bool Load(RequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->GetAssetTypeID() == Texture::GetStaticAssetTypeID());

        Texture* pTexture = aln::New<Texture>();

        /// @todo Also handle 2D/3D Textures here
        /// @todo Replace with uint32_t
        int width, height;
        Vector<std::byte> data;

        archive >> width;
        archive >> height;
        archive >> data;

        /// @note Only RGBA8 supported for now
        auto vkFormat = vk::Format::eR8G8B8A8Srgb;
        auto mipLevels = static_cast<uint32_t>(Maths::Floor(Maths::Log2(Maths::Max((float) width, (float) height)))) + 1;

        // Copy data to staging buffer
        // GPUBuffer stagingBuffer(m_pRenderEngine, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, data);
        m_stagingBuffer.Copy(data);

        // TODO: Move what can be moved to the transfer queue
        auto pCB = ctx.GetGraphicsCommandBuffer();
        pTexture->m_image.InitializeFromBuffer(m_pRenderEngine, pCB, m_stagingBuffer, width, height, mipLevels, vkFormat);

        pTexture->m_image.AddView();
        pTexture->m_image.AddSampler();
        pTexture->m_image.SetDebugName("Material Texture"); // todo: add id name

        pRecord->SetAsset(pTexture);

        return true;
    }

    void Unload(AssetRecord* pRecord) override
    {
        auto pTexture = pRecord->GetAsset<Texture>();
        pTexture->m_image.Shutdown();
    }
};

} // namespace aln