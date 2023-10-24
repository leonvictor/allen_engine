#pragma once

#include <vulkan/vulkan.hpp>

#include <graphics/rendering/render_target.hpp>
#include <graphics/rendering/renderer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/window.hpp>

namespace aln
{

// UI Renderer's role is to draw the editor windows. It actually renders to the the swapchain images which can be presented to the surface
/// @todo: Maybe inherit from a "swapchain renderer" ? So that we can also display the main game without the editor
class EditorRenderer : public IRenderer
{
  private:
    Swapchain* m_pSwapchain;

  public:
    void Initialize(RenderEngine* pRenderEngine) override
    {
        m_pSwapchain = &pRenderEngine->GetWindow()->GetSwapchain();
        m_pSwapchain->AddResizeCallback(std::bind(&EditorRenderer::Resize, this, std::placeholders::_1, std::placeholders::_2));

        m_pRenderEngine = pRenderEngine;
        auto windowSize = m_pRenderEngine->GetWindow()->GetFramebufferSize();
        auto colorImageFormat = m_pRenderEngine->GetWindow()->GetSwapchain().GetImageFormat();

        // Create Renderpass
        // This is the default render pass
        m_renderpass = RenderPass(m_pRenderEngine, windowSize.width, windowSize.height);
        m_renderpass.AddColorAttachment(colorImageFormat);
        m_renderpass.AddDepthAttachment();
        m_renderpass.AddColorResolveAttachment(colorImageFormat);

        auto& subpass = m_renderpass.AddSubpass();
        subpass.ReferenceColorAttachment(0);
        subpass.ReferenceDepthAttachment(1);
        subpass.ReferenceResolveAttachment(2);

        // Add a subpass dependency to ensure the render pass will wait for the right stage
        // We need to wait for the image to be acquired before transitionning to it
        vk::SubpassDependency dep = {
            .srcSubpass = vk::SubpassExternal,                                 // The implicit subpass before or after the render pass
            .dstSubpass = 0,                                                   // Target subpass index (we have only one)
            .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput, // Stage to wait on
            .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits::eNone,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        };

        m_renderpass.AddSubpassDependency(dep);
        m_renderpass.Create();

        auto swapchainImageCount = m_pSwapchain->GetImageCount();
        m_renderTargets.resize(swapchainImageCount);
        for (auto imageIdx = 0; imageIdx < swapchainImageCount; imageIdx++)
        {
            auto& renderTarget = m_renderTargets[imageIdx];

            // Wrap swapchain image
            renderTarget.m_resolveImage.Initialize(
                m_pRenderEngine,
                m_pSwapchain->GetImage(imageIdx),
                m_pSwapchain->GetImageFormat());
            renderTarget.m_resolveImage.AddView(vk::ImageAspectFlagBits::eColor);
            renderTarget.m_resolveImage.SetDebugName("Swapchain Target");

            // Depth image
            renderTarget.m_depthImage.Initialize(
                m_pRenderEngine,
                windowSize.width,
                windowSize.height,
                1,
                m_pRenderEngine->GetMSAASamples(),
                m_pRenderEngine->FindDepthFormat(),
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment);
            renderTarget.m_depthImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            renderTarget.m_depthImage.AddView(vk::ImageAspectFlagBits::eDepth);
            renderTarget.m_depthImage.SetDebugName("Renderer Depth Attachment");

            // Multisampling image
            renderTarget.m_multisamplingImage.Initialize(
                m_pRenderEngine,
                windowSize.width,
                windowSize.height,
                1,
                m_pRenderEngine->GetMSAASamples(),
                m_pSwapchain->GetImageFormat(),
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment);

            renderTarget.m_multisamplingImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            renderTarget.m_multisamplingImage.AddView(vk::ImageAspectFlagBits::eColor);
            renderTarget.m_multisamplingImage.SetDebugName("Renderer Color Attachment");

            //  Framebuffer
            Vector<vk::ImageView> attachments = {
                renderTarget.m_multisamplingImage.GetVkView(),
                renderTarget.m_depthImage.GetVkView(),
                renderTarget.m_resolveImage.GetVkView(),
            };

            vk::FramebufferCreateInfo framebufferInfo = {
                .renderPass = m_renderpass.GetVkRenderPass(),
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = windowSize.width,
                .height = windowSize.height,
                .layers = 1,
            };

            renderTarget.m_framebuffer = m_pRenderEngine->GetVkDevice().createFramebuffer(framebufferInfo).value;
            
            // Sync
            renderTarget.m_renderFinished = m_pRenderEngine->GetVkDevice().createSemaphore({}).value;
        };
    }

    void Shutdown() override
    {
        for (auto& renderTarget : m_renderTargets)
        {
            m_pRenderEngine->GetVkDevice().destroyFramebuffer(renderTarget.m_framebuffer);
            renderTarget.m_depthImage.Shutdown();
            renderTarget.m_multisamplingImage.Shutdown();
            renderTarget.m_resolveImage.Shutdown();
            m_pRenderEngine->GetVkDevice().destroySemaphore(renderTarget.m_renderFinished);
        }
    
        m_renderpass.Shutdown();
    }

    void Resize(uint32_t width, uint32_t height)
    {
        assert(false); // TODO
       /* CreateInternal(
            m_pSwapchain->GetDevice(),
            width,
            height,
            m_pSwapchain->GetImageFormat());*/
    }

    void StartFrame(TransientCommandBuffer& cb, const RenderContext& ctx)
    {
        ZoneScoped;

        const auto currentFrameIdx = m_pRenderEngine->GetCurrentFrameIdx();

        // Acquire an image from the swap chain
        auto renderTargetIdx = m_pSwapchain->AcquireNextImage();
        auto& renderTarget = m_renderTargets[renderTargetIdx];

        RenderPass::Context renderPassCtx = {
            .commandBuffer = (vk::CommandBuffer&) cb,
            .framebuffer = renderTarget.m_framebuffer,
            .backgroundColor = ctx.backgroundColor,
        };

        m_renderpass.Begin(renderPassCtx);
    }

    void EndFrame(TransientCommandBuffer& cb)
    {
        cb->endRenderPass();
    }

    RenderTarget& GetCurrentRenderTarget() { return m_renderTargets[m_pSwapchain->GetCurrentImageIdx()]; }
};
} // namespace aln