#pragma once

#include <vulkan/vulkan.hpp>

#include "../resources/image.hpp"
#include "render_target.hpp"
#include "renderer.hpp"

namespace aln::vkg::render
{

class OfflineRenderer : public IRenderer
{
  private:
    uint32_t m_nTargetImages;

    void CreateTargetImages() override
    {
        m_targetImages.clear();

        auto cbs = m_pDevice->GetGraphicsCommandPool().BeginSingleTimeCommands();

        for (uint8_t i = 0; i < m_nTargetImages; ++i)
        {
            auto target = std::make_shared<Image>(
                m_pDevice, m_width, m_height, 1,
                vk::SampleCountFlagBits::e1,
                m_colorImageFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled);
            target->Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            target->AddView(vk::ImageAspectFlagBits::eColor);
            target->AddSampler();
            target->SetDebugName("Offline Renderer Target");

            target->TransitionLayout(cbs[0], vk::ImageLayout::eGeneral);
            m_pDevice->SetDebugUtilsObjectName(target->GetVkImage(), "Offline Render Target Image (" + std::to_string(i) + ")");

            m_targetImages.push_back(target);
        }

        m_pDevice->GetGraphicsCommandPool().EndSingleTimeCommands(cbs);
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

    void CreateRenderpass() override
    {
        m_renderpass = RenderPass(m_pDevice, m_width, m_height);
        m_renderpass.AddColorAttachment(m_colorImageFormat);
        m_renderpass.AddDepthAttachment();
        m_renderpass.AddColorResolveAttachment(m_colorImageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

        auto& subpass = m_renderpass.AddSubpass();
        subpass.ReferenceColorAttachment(0);
        subpass.ReferenceDepthAttachment(1);
        subpass.ReferenceResolveAttachment(2);

        // Add a subpass dependency to ensure the render pass will wait for the right stage
        // We need to wait for the image to be acquired before transitionning to it
        vk::SubpassDependency dep;
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;                                 // The implicit subpass before or after the render pass
        dep.dstSubpass = 0;                                                   // Target subpass index (we have only one)
        dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // Stage to wait on
        dep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dep.srcAccessMask = vk::AccessFlagBits(0);
        dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        m_renderpass.AddSubpassDependency(dep);
        m_renderpass.Create();
    }

  public:
    OfflineRenderer() {}

    void Create(std::shared_ptr<Device> pDevice, int width, int height, int nTargetImages, vk::Format colorImageFormat)
    {
        m_nTargetImages = nTargetImages;
        CreateInternal(pDevice, width, height, colorImageFormat);
        CreatePipelines();
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
} // namespace aln::vkg::render