#pragma once

#include "../device.hpp"
#include "../imgui.hpp"
#include "../pipeline.hpp"
#include "../render_pass.hpp"
#include "../resources/buffer.hpp"
#include "../resources/image.hpp"
#include "../swapchain.hpp"
#include "render_target.hpp"

#include <common/colors.hpp>
#include <common/containers/vector.hpp>
#include <common/hash_vector.hpp>

#include <config/path.h>
#include <vulkan/vulkan.hpp>

#include <cstring>

namespace aln
{

// fwd
class StaticMeshComponent;
class SkeletalMeshComponent;
class Light;
class Mesh;

namespace vkg::render
{

using vkg::resources::Image;

struct FrameSync
{
    vk::UniqueSemaphore imageAvailable;
    vk::UniqueSemaphore renderFinished;
    vk::UniqueFence inFlight;
};

struct RenderContext
{
    aln::RGBAColor backgroundColor = {0, 0, 0, 255};
};

/// TODO: Modify the API to allow offline rendering. The idea would be to render to a texture which we can display onto an imgui window.
/// We need to be careful here, as it'd be good to keep the possibility to directly render to the swapchain images
/// For when we'll create game builds for example.
/// We also need to keep frame buffering in the process.
///
/// How would that look like ? Pull out swapchain, and provide the image to render to as arguments ?
/// Or various subclasses with similar APIs (OfflineRenderer, SwapchainRenderer) ? Maybe GameRender, EditorRenderer ?

/// @brief Renderer instance used to draw the scenes and the UI.
class IRenderer
{
    enum State
    {
        Uninitialized,
        Initialized
    };

  private:
    /// @brief Override in child classes to specify how to retrieve the target images.
    virtual void CreateTargetImages() = 0;
    virtual RenderTarget& GetNextTarget() = 0;

  protected:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    static constexpr uint32_t MAX_MODELS = 1000;
    static constexpr uint32_t MAX_SKINNING_TRANSFORMS = 255 * 50;

    State m_state = State::Uninitialized;

    Device* m_pDevice;

    uint32_t m_currentFrameIndex = 0; // Frame index
    uint32_t m_activeImageIndex = 0;  // Active image index

    vk::Format m_colorImageFormat;

    // Sync objects
    Vector<FrameSync> m_frames;

    // Target images
    Vector<std::shared_ptr<Image>> m_targetImages;
    Vector<RenderTarget> m_renderTargets;

    Image m_colorImage; // Color image attachment for multisampling.
    Image m_depthImage; // Depth image attachment.

    RenderPass m_renderpass;
    uint32_t m_width, m_height;

    virtual void CreateInternal(Device* pDevice, uint32_t width, uint32_t height, vk::Format colorImageFormat)
    {
        m_pDevice = pDevice;
        m_width = width;
        m_height = height;
        m_colorImageFormat = colorImageFormat;

        // Create color attachment resources for multisampling
        m_colorImage = Image(
            m_pDevice,
            width,
            height,
            1,
            m_pDevice->GetMSAASamples(),
            m_colorImageFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment);
        m_colorImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_colorImage.AddView(vk::ImageAspectFlagBits::eColor);
        m_colorImage.SetDebugName("Renderer Color Attachment");

        // Create depth attachment ressources
        m_depthImage = Image(
            m_pDevice,
            width,
            height,
            1,
            m_pDevice->GetMSAASamples(),
            m_pDevice->FindDepthFormat(),
            vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment);
        m_depthImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_depthImage.AddView(vk::ImageAspectFlagBits::eDepth);
        m_depthImage.SetDebugName("Renderer Depth Attachment");

        CreateRenderpass();

        // Create the render targets
        CreateTargetImages();
        auto commandBuffers = m_pDevice->GetGraphicsCommandPool().AllocateCommandBuffersUnique(m_targetImages.size());

        m_renderTargets.clear();
        for (uint32_t i = 0; i < m_targetImages.size(); i++)
        {
            Vector<vk::ImageView> attachments = {
                    m_colorImage.GetVkView(),
                    m_depthImage.GetVkView(),
                    m_targetImages[i]->GetVkView(),
                };

            vk::FramebufferCreateInfo framebufferInfo = {
                    .renderPass = m_renderpass.GetVkRenderPass(),
                    .attachmentCount = static_cast<uint32_t>(attachments.size()),
                    .pAttachments = attachments.data(),
                    .width = width,
                    .height = height,
                    .layers = 1, // Nb of layers in image array.
                };

            RenderTarget rt = {
                    .index = i,
                    .framebuffer = m_pDevice->GetVkDevice().createFramebufferUnique(framebufferInfo).value,
                    .commandBuffer = std::move(commandBuffers[i]),
                    .fence = vk::Fence(),
                };

            m_renderTargets.push_back(std::move(rt));
        }

        // Create sync objects
        // Create fences in the signaled state
        vk::FenceCreateInfo fenceInfo = {.flags = vk::FenceCreateFlagBits::eSignaled};

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_frames.push_back({
                .imageAvailable = m_pDevice->GetVkDevice().createSemaphoreUnique({}, nullptr).value,
                .renderFinished = m_pDevice->GetVkDevice().createSemaphoreUnique({}, nullptr).value,
                .inFlight = m_pDevice->GetVkDevice().createFenceUnique(fenceInfo, nullptr).value,
            });
        }
    }

    /// @brief Configure and create the render pass. Override this function in derived renderer if necessary.
    virtual void CreateRenderpass()
    {
        // This is the default render pass
        m_renderpass = RenderPass(m_pDevice, m_width, m_height);
        m_renderpass.AddColorAttachment(m_colorImageFormat);
        m_renderpass.AddDepthAttachment();
        m_renderpass.AddColorResolveAttachment(m_colorImageFormat);

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
    }

  public:
    virtual void BeginFrame(const RenderContext& ctx)
    {
        ZoneScoped;

        // TODO: Handle VK_TIMEOUT and VK_OUT_OF_DATE_KHR
        m_pDevice->GetVkDevice().waitForFences(m_frames[m_currentFrameIndex].inFlight.get(), vk::True, UINT64_MAX);

        // Acquire an image from the swap chain
        auto& image = GetNextTarget();

        // Check if a previous frame is using the image
        if (image.fence)
        {
            m_pDevice->GetVkDevice().waitForFences(image.fence, vk::True, UINT64_MAX);
        }

        // Mark the image as now being in use by this frame
        image.fence = m_frames[m_currentFrameIndex].inFlight.get();

        // Start recording command on this image
        vk::CommandBufferBeginInfo cbBeginInfo;
        image.commandBuffer->begin(cbBeginInfo);

        // Start a render pass
        // TODO: Pass a RenderTarget
        RenderPass::Context renderPassCtx = {
            .commandBuffer = image.commandBuffer.get(),
            .framebuffer = image.framebuffer.get(),
            .backgroundColor = ctx.backgroundColor,
        };

        m_renderpass.Begin(renderPassCtx);
    }

    virtual void EndFrame()
    {
        ZoneScoped;

        auto& cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();
        cb.endRenderPass();
        cb.end();

        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo submitInfo = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_frames[m_currentFrameIndex].imageAvailable.get(),
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &cb,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &m_frames[m_currentFrameIndex].renderFinished.get(),
        };

        m_pDevice->GetVkDevice().resetFences(m_frames[m_currentFrameIndex].inFlight.get());
        // TODO: It would be better to pass the semaphores and cbs directly to the queue class
        // but we need a mechanism to avoid having x versions of the method for (single elements * arrays * n_occurences)
        // vulkan arraywrappers ?
        m_pDevice->GetGraphicsQueue().Submit(submitInfo, m_frames[m_currentFrameIndex].inFlight.get());

        m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    Device* GetDevice()
    {
        // TODO: Get rid of all the references.
        std::cout << "Warning: accessing device outside of renderer context." << std::endl;
        return m_pDevice;
    }

    RenderPass& GetRenderPass()
    {
        // TODO: Get rid of all the references.
        std::cout << "Warning: accessing renderpass outside of renderer context." << std::endl;
        return m_renderpass;
    }

    vk::Extent2D GetExtent() const { return {m_width, m_height}; }
    uint32_t GetFramesInFlightCount() const { return m_targetImages.size(); }
    uint32_t GetActiveFrameIndex() const { return m_activeImageIndex; }

    const Image* GetFrameImage(uint32_t imageIdx) const
    {
        assert(imageIdx < m_targetImages.size());
        return m_targetImages[imageIdx].get();
    }

    // TODO Combine Image in RenderTarget
    RenderTarget& GetActiveRenderTarget()
    {
        return m_renderTargets[m_activeImageIndex];
    }

    std::shared_ptr<Image> GetActiveImage()
    {
        return m_targetImages[m_activeImageIndex];
    }
};
} // namespace vkg::render
} // namespace aln