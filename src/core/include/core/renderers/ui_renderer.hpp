#pragma once

#include <vulkan/vulkan.hpp>

#include <graphics/rendering/render_target.hpp>
#include <graphics/rendering/renderer.hpp>
#include <graphics/resources/image.hpp>

namespace aln
{

class UIRenderer : public render::IRenderer
{
  private:
    Swapchain* m_pSwapchain;

    void CreateTargetImages() override
    {
        m_targetImages.clear();
        // Create the swapchain images
        auto result = m_pRenderEngine->GetVkDevice().getSwapchainImagesKHR(m_pSwapchain->GetVkSwapchain());
        auto& images = result.value;

        for (size_t i = 0; i < images.size(); i++)
        {
            auto target = std::make_shared<resources::Image>(m_pRenderEngine, images[i], m_colorImageFormat);
            target->AddView(vk::ImageAspectFlagBits::eColor);
            target->SetDebugName("Swapchain Target");
            m_targetImages.push_back(target);
        };
    }

    render::RenderTarget& GetNextTarget() override
    {
        m_activeImageIndex = m_pSwapchain->AcquireNextImage(m_frameSync[m_pRenderEngine->GetCurrentFrameIdx()].imageAvailable.get());
        return m_renderTargets[m_activeImageIndex];
    }

  public:
    void Initialize(Swapchain* pSwapchain)
    {
        m_pSwapchain = pSwapchain;

        // Hook a callback that will trigger when the associated swapchain is resized
        m_pSwapchain->AddResizeCallback(std::bind(&UIRenderer::Resize, this, std::placeholders::_1, std::placeholders::_2));
        CreateInternal(pSwapchain->GetDevice(),
            pSwapchain->GetWidth(),
            pSwapchain->GetHeight(),
            pSwapchain->GetImageFormat());
    }

    void Shutdown()
    {
        for (auto& targetImage : m_targetImages)
        {
            targetImage->Shutdown();
            targetImage.reset();
        }
        m_targetImages.clear();

        IRenderer::Shutdown();
    }

    void Resize(uint32_t width, uint32_t height)
    {
        CreateInternal(
            m_pSwapchain->GetDevice(),
            width,
            height,
            m_pSwapchain->GetImageFormat());
    }

    // TODO: Signal the semaphore manually
    void EndFrame()
    {
        ZoneScoped;

        auto& cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();
        cb.endRenderPass();
        cb.end();

        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        const auto currentFrameIdx = m_pRenderEngine->GetCurrentFrameIdx();

        vk::SubmitInfo submitInfo = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_frameSync[currentFrameIdx].imageAvailable.get(),
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &cb,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &m_frameSync[currentFrameIdx].renderFinished.get(),
        };

        m_pRenderEngine->GetVkDevice().resetFences(m_frameSync[currentFrameIdx].inFlight.get());
        // TODO: It would be better to pass the semaphores and cbs directly to the queue class
        // but we need a mechanism to avoid having x versions of the method for (single elements * arrays * n_occurences)
        // vulkan arraywrappers ?
        m_pRenderEngine->GetGraphicsQueue().Submit(submitInfo, m_frameSync[currentFrameIdx].inFlight.get());

        m_pSwapchain->Present(m_frameSync[currentFrameIdx].renderFinished.get());
    }
};
} // namespace aln