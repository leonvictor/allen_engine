#pragma once

#include "../device.hpp"
#include "../swapchain.hpp"
#include "render_target.hpp"
#include "renderer.hpp"

#include <functional>
#include <vulkan/vulkan.hpp>

namespace vkg
{

class SwapchainRenderer : public IRenderer
{
  private:
    Swapchain* m_pSwapchain;

    void CreateTargetImages() override
    {
        m_targetImages.clear();
        // Create the swapchain images
        auto images = m_pDevice->GetVkDevice().getSwapchainImagesKHR(m_pSwapchain->GetVkSwapchain());

        for (size_t i = 0; i < images.size(); i++)
        {
            auto target = std::make_shared<vkg::Image>(m_pDevice, images[i], m_colorImageFormat);
            target->AddView(vk::ImageAspectFlagBits::eColor);
            m_targetImages.push_back(target);
        };
    }

    RenderTarget& GetNextTarget() override
    {
        m_activeImageIndex = m_pSwapchain->AcquireNextImage(m_frames[m_currentFrameIndex].imageAvailable.get());
        return m_renderTargets[m_activeImageIndex];
    }

  public:
    SwapchainRenderer() {}

    void Create(Swapchain* pSwapchain)
    {
        m_pSwapchain = pSwapchain;

        // Hook a callback that will trigger when the associated swapchain is resized
        m_pSwapchain->AddResizeCallback(std::bind(&SwapchainRenderer::Resize, this, std::placeholders::_1, std::placeholders::_2));
        CreateInternal(pSwapchain->GetDevice(),
            pSwapchain->GetWidth(),
            pSwapchain->GetHeight(),
            pSwapchain->GetImageFormat());
        CreatePipelines();
    }

    void Resize(uint32_t width, uint32_t height)
    {
        CreateInternal(
            m_pSwapchain->GetDevice(),
            width,
            height,
            m_pSwapchain->GetImageFormat());
    }

    // TODO: De-duplicate
    void EndFrame()
    {
        auto& cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();
        cb.endRenderPass();
        cb.end();

        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo submitInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &m_frames[m_currentFrameIndex].imageAvailable.get();
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_frames[m_currentFrameIndex].renderFinished.get();

        m_pDevice->GetVkDevice().resetFences(m_frames[m_currentFrameIndex].inFlight.get());
        // TODO: It would be better to pass the semaphores and cbs directly to the queue class
        // but we need a mechanism to avoid having x versions of the method for (single elements * arrays * n_occurences)
        // vulkan arraywrappers ?
        m_pDevice->GetGraphicsQueue()
            .Submit(submitInfo, m_frames[m_currentFrameIndex].inFlight.get());

        m_pSwapchain->Present(m_frames[m_currentFrameIndex].renderFinished.get());

        m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }
};
} // namespace vkg