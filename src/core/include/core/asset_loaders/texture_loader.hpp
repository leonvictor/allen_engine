#pragma once

#include <assets/asset.hpp>
#include <assets/loader.hpp>
#include <assets/request_context.hpp>
#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>

#include "../texture.hpp"

namespace aln
{

class TextureLoader : public IAssetLoader
{
  private:
    RenderEngine* m_pRenderEngine;

  public:
    TextureLoader(RenderEngine* pRenderEngine)
    {
        m_pRenderEngine = pRenderEngine;
    }


    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->GetAssetTypeID() == Texture::GetStaticAssetTypeID());

        Texture* pTexture = aln::New<Texture>();

        // TODO: Read directly from disk to gpu buffer
        // TODO: Also handle 2D/3D Textures here
        // TODO: Replace with uint32_t
        int width, height;
        Vector<std::byte> data;

        archive >> width;
        archive >> height;
        archive >> data;

        auto pGraphicsQueueSubmission = ctx.GetGraphicsQueueSubmission();
        auto pTransferQueueSubmission = ctx.GetTransferQueueSubmission();

        auto mipLevels = static_cast<uint32_t>(Maths::Floor(Maths::Log2(Maths::Max((float) width, (float) height)))) + 1;
        pTexture->m_image.Initialize(m_pRenderEngine,
            width, height,
            mipLevels,
            vk::SampleCountFlagBits::e1,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
            1,
            {},
            vk::ImageLayout::eUndefined,
            vk::ImageType::e2D);

        pTexture->m_image.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);

        // Transition layout to transferDstOptimal
        /*vk::HostImageLayoutTransitionInfoEXT transitionInfo = {
            .image = pTexture->m_image.GetVkImage(),
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
        };

        m_pRenderEngine->GetVkDevice().transitionImageLayoutEXT(transitionInfo);*/
        pTexture->m_image.TransitionLayout((vk::CommandBuffer) *pTransferQueueSubmission->GetCommandBuffer(), vk::ImageLayout::eTransferDstOptimal);
        
        // Upload to GPU
        auto [pUploadFinishedSemaphore, uploadFinishedSemaphoreValue]  = ctx.UploadImageThroughStaging(data, pTexture->m_image);

        // Optionnaly generate mipmaps
        if (mipLevels > 1)
        {
            // Blit commands can only be executed on graphics queues
            // Wait for the upload to be complete before starting
            pGraphicsQueueSubmission->WaitSemaphore(pUploadFinishedSemaphore, uploadFinishedSemaphoreValue);
            pTexture->m_image.GenerateMipMaps((vk::CommandBuffer) *pGraphicsQueueSubmission->GetCommandBuffer(), mipLevels);
        }
        else
        {
            // Otherwise, transfer image layout to shader readonly on the transfer queue
            pTexture->m_image.TransitionLayout((vk::CommandBuffer) *pTransferQueueSubmission->GetCommandBuffer(), vk::ImageLayout::eShaderReadOnlyOptimal);
        }

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