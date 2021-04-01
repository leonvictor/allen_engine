#pragma once

#include "../utils/observer.hpp"
#include "resources/image.hpp"
#include "swapchain.hpp"

#include <assert.h>
#include <vulkan/vulkan.hpp>

namespace vkg
{
/// @brief Wrapper around the vulkan render pass object. Also acts as a sort of factory.
class RenderPass : public IObserver
{

    enum State
    {
        Uninitialized,
        Ready,
        Running,
    };

  public:
    RenderPass() {}
    RenderPass(std::shared_ptr<Device> pDevice)
    {
        m_pDevice = pDevice;
        // Default clear values.
        // TODO: Make it possible to customize clear values
        m_clearValues[0].color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        m_clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    }

    void Create(vkg::Swapchain* pTargetSwapchain)
    {
        // TODO: Make more modular by allowing the user to gradually add attachments.
        assert(!m_created);
        assert(m_pDevice);

        m_extent = pTargetSwapchain->GetExtent();

        // Create color attachment resources
        m_colorImage = vkg::Image(m_pDevice, pTargetSwapchain->GetWidth(), pTargetSwapchain->GetHeight(), 1, m_pDevice->GetMSAASamples(), pTargetSwapchain->GetImageFormat(),
                                  vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  vk::ImageAspectFlagBits::eColor);
        // TODO: Decide how to set the debug names. Either pass a debug name in the constructor or add a
        // image.SetDebugName() method.
        // m_pDevice->setDebugUtilsObjectName(colorImage.image.get(), "Swapchain color image");
        // m_pDevice->setDebugUtilsObjectName(colorImage.view.get(), "Color Image View");

        // Create depth attachment ressources
        m_depthImage = vkg::Image(m_pDevice, pTargetSwapchain->GetWidth(), pTargetSwapchain->GetHeight(), 1, m_pDevice->GetMSAASamples(),
                                  m_pDevice->FindDepthFormat(), vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  vk::ImageAspectFlagBits::eDepth);
        // m_pDevice->setDebugUtilsObjectName(dekpthImage->image.get(), "Swapchain depth image");
        vk::AttachmentDescription colorAttachment{
            .format = pTargetSwapchain->GetImageFormat(),
            .samples = m_pDevice->GetMSAASamples(),
            // Color and depth data
            .loadOp = vk::AttachmentLoadOp::eClear,   // What to do with the data before ...
            .storeOp = vk::AttachmentStoreOp::eStore, // ... and after rendering
            // Stencil data
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare, // We don't have stencils for now
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            // Images need to be transitioned to specific layout that are suitable for the op that they're going to be involved in next.
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::eColorAttachmentOptimal,
        };

        vk::AttachmentDescription depthAttachment{
            .format = m_pDevice->FindDepthFormat(),
            .samples = m_pDevice->GetMSAASamples(),
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare, // Depth data is not used after drawing has finished
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };

        vk::AttachmentDescription colorAttachmentResolve{
            .format = pTargetSwapchain->GetImageFormat(),
            .samples = vk::SampleCountFlagBits::e1,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::ePresentSrcKHR,
        };

        // Create a subpass referencing the attachments.
        // TODO: Make more customizable
        vk::AttachmentReference colorAttachmentRef{
            .attachment = 0,
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
        };

        vk::AttachmentReference depthAttachmentRef{
            .attachment = 1,
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };

        vk::AttachmentReference colorAttachmentResolveRef{
            .attachment = 2,
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
        };

        vk::SubpassDescription subpass{
            .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef, // The index of the attachment is directly referenced in the fragment shader ( layout(location = 0) )...
            .pResolveAttachments = &colorAttachmentResolveRef,
            .pDepthStencilAttachment = &depthAttachmentRef,
        };

        // Add a subpass dependency to ensure the render pass will wait for the right stage
        // We need to wait for the image to be acquired before transitionning to it
        vk::SubpassDependency subpassDependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,                                 // The implicit subpass before or after the render pass
            .dstSubpass = 0,                                                   // Target subpass index (we have only one)
            .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput, // Stage to wait on
            .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits(0),
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        };

        std::array<vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        vk::RenderPassCreateInfo renderPassInfo{
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &subpassDependency,
        };

        m_vkRenderPass = m_pDevice->GetVkDevice().createRenderPassUnique(renderPassInfo);
        CreateFramebuffers(pTargetSwapchain);

        // Attach itself to a swapchain to be notified when the target changes.
        pTargetSwapchain->Attach(this);

        m_created = true;
    }

    /// @brief Begin a render pass.
    /// @param frameIndex: index of the swapchain frame to render to.
    void Begin(vk::CommandBuffer& commandBuffer, uint32_t frameIndex)
    {
        assert(m_created);

        vk::RenderPassBeginInfo renderPassInfo{
            .renderPass = m_vkRenderPass.get(),
            .framebuffer = m_framebuffers[frameIndex].get(),
            .renderArea = {
                .offset = {0, 0},
                .extent = m_extent,
            },
            .clearValueCount = static_cast<uint32_t>(m_clearValues.size()),
            .pClearValues = m_clearValues.data(),
        };
        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    }

    void End(vk::CommandBuffer& cb)
    {
        cb.endRenderPass();
    }

    inline vk::RenderPass& GetVkRenderPass() { return m_vkRenderPass.get(); }

    /// @brief Callback called when the target swapchain has changed. Recreates the framebuffers.
    void Update(IObservable* pObserved) override
    {
        auto pSwapchain = dynamic_cast<vkg::Swapchain*>(pObserved);
        // TODO: Make sure this is the same target
        assert(pSwapchain != nullptr);
        CreateFramebuffers(pSwapchain);
    }

  private:
    // TODO: Status with more details (uninitialized, active or not)
    // Whether the renderpass has been created yet.
    bool m_created = false;
    std::shared_ptr<Device> m_pDevice;
    std::array<vk::ClearValue, 2> m_clearValues;
    std::vector<vk::UniqueFramebuffer> m_framebuffers;

    vk::Extent2D m_extent;

    // TODO: Decide where attachments should be.
    // For now they're fine here since only one drawing operation is active at a time.
    Image m_colorImage; // Used as an attachment for multisampling
    Image m_depthImage; // Used as an attachment to resolve image depth.

    // Wrapped vulkan renderpass
    vk::UniqueRenderPass m_vkRenderPass;

    // TODO: How do we handle multiple renderpasses ?

    void CreateFramebuffers(const vkg::Swapchain* pSwapchain)
    {
        m_framebuffers.clear();

        for (auto& targetImage : pSwapchain->GetImages())
        {
            // TODO: Where do colorImage and depthImage live ?
            std::vector<vk::ImageView> attachments = {
                m_colorImage.GetVkView(),
                m_depthImage.GetVkView(),
                targetImage.view.get(),
            };

            AddFramebuffer(attachments);
        }
    }

    void AddFramebuffer(const std::vector<vk::ImageView>& attachments)
    {
        // TODO: Assert status is initialized
        vk::FramebufferCreateInfo framebufferInfo{
            .renderPass = m_vkRenderPass.get(),
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = m_extent.width,
            .height = m_extent.height,
            .layers = 1, // Nb of layers in image array.
        };

        m_framebuffers.emplace_back(m_pDevice->GetVkDevice().createFramebufferUnique(framebufferInfo));
    }
};
} // namespace vkg