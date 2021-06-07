#pragma once

#include <vulkan/vulkan.hpp>

#include "../resources/texture.hpp"
#include "render_target.hpp"
#include "renderer.hpp"

namespace vkg
{

class OfflineRenderer : public IRenderer
{
  private:
    uint32_t m_nTargetImages;

    void CreateTargetImages() override
    {
        m_targetImages.clear();

        for (int i = 0; i < m_nTargetImages; ++i)
        {

            m_targetImages.push_back(
                std::make_shared<Texture>(
                    m_pDevice, m_width, m_height, 1,
                    vk::SampleCountFlagBits::e1,
                    m_colorImageFormat,
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::eGeneral));
        }
    }

    RenderTarget& GetNextTarget() override
    {
        m_activeImageIndex++;
        if (m_activeImageIndex >= m_nTargetImages)
        {
            m_activeImageIndex = 0;
        }

        return m_renderTargets[m_activeImageIndex];
    }

  public:
    OfflineRenderer() {}

    void Create(std::shared_ptr<Device> pDevice, int width, int height, int nTargetImages, vk::Format colorImageFormat)
    {
        m_nTargetImages = nTargetImages;
        CreateInternal(pDevice, width, height, colorImageFormat);
    }

    // TODO: Split Creation so we can customize the renderpasses here

    // TODO: Signal the semaphore manually
    void EndFrame()
    {
        auto& cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();
        cb.endRenderPass();
        cb.end();

        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;

        m_pDevice->GetVkDevice().resetFences(m_frames[m_currentFrameIndex].inFlight.get());
        // TODO: It would be better to pass the semaphores and cbs directly to the queue class
        // but we need a mechanism to avoid having x versions of the method for (single elements * arrays * n_occurences)
        // vulkan arraywrappers ?
        m_pDevice->GetGraphicsQueue()
            .Submit(submitInfo, m_frames[m_currentFrameIndex].inFlight.get());

        m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }
};
} // namespace vkg