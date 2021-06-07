#pragma once

#include "../../light.cpp"
#include "../../scene_object.cpp"
#include "../../skybox.cpp"

#include "../device.hpp"
#include "../imgui.hpp"
#include "../pipeline.hpp"
#include "../render_pass2.hpp"
#include "../swapchain.hpp"
#include "../window.hpp"
#include "render_target.hpp"

#include <vulkan/vulkan.hpp>

namespace vkg
{
struct FrameSync
{
    vk::UniqueSemaphore imageAvailable;
    vk::UniqueSemaphore renderFinished;
    vk::UniqueFence inFlight;
};

/// TODO: Modify the API to allow offline rendering. The idea would be to render to a texture which we can display onto an imgui window.
/// We need to be careful here, as it'd be good to keep the possibility to directly render to the swapchain images
/// For when we'll create game builds for example.
/// We also need to keep frame buffering in the process.
///
/// How would that look like ? Pull out swapchain, and provide the image to render to as arguments ?
/// Or various subclasses with similar APIs (OfflineRenderer, SwapchainRenderer) ?

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
    const int MAX_FRAMES_IN_FLIGHT = 2;

    State m_state = State::Uninitialized;

    std::shared_ptr<Device> m_pDevice;

    uint32_t m_currentFrameIndex = 0; // Frame index
    uint32_t m_activeImageIndex = 0;  // Active image index

    vk::Format m_colorImageFormat;

    // Sync objects
    std::vector<FrameSync> m_frames;

    // Target images
    std::vector<std::shared_ptr<Image>> m_targetImages;
    std::vector<RenderTarget> m_renderTargets;

    Image m_colorImage; // Color image attachment for multisampling.
    Image m_depthImage; // Depth image attachment.

    RenderPass m_renderpass;
    uint32_t m_width, m_height;

    struct
    {
        Pipeline objects;
        Pipeline skybox;
    } pipelines;

    IRenderer() {}

    void CreateInternal(std::shared_ptr<Device> pDevice, uint32_t width, uint32_t height, vk::Format colorImageFormat)
    {
        // Necessary parameters
        // TODO: Grab them from somewhere
        m_pDevice = pDevice;
        m_width = width;
        m_height = height;
        m_colorImageFormat = colorImageFormat;

        // Create color attachment resources for multisampling
        m_colorImage = vkg::Image(m_pDevice, width, height, 1, m_pDevice->GetMSAASamples(), m_colorImageFormat,
                                  vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  vk::ImageAspectFlagBits::eColor);

        // Create depth attachment ressources
        m_depthImage = vkg::Image(m_pDevice, width, height, 1, m_pDevice->GetMSAASamples(),
                                  m_pDevice->FindDepthFormat(), vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  vk::ImageAspectFlagBits::eDepth);

        CreateRenderpass();

        // Create the render targets
        CreateTargetImages();
        auto commandBuffers = m_pDevice->GetGraphicsCommandPool().AllocateCommandBuffersUnique(m_targetImages.size());

        for (uint32_t i = 0; i < m_targetImages.size(); i++)
        {
            std::vector<vk::ImageView> attachments = {
                m_colorImage.GetVkView(),
                m_depthImage.GetVkView(),
                m_targetImages[i]->GetVkView(),
            };

            vk::FramebufferCreateInfo framebufferInfo;
            framebufferInfo.renderPass = m_renderpass.GetVkRenderPass();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1; // Nb of layers in image array.

            RenderTarget rt = {
                .index = i,
                .framebuffer = m_pDevice->GetVkDevice().createFramebufferUnique(framebufferInfo),
                .commandBuffer = std::move(commandBuffers[i]),
                .fence = vk::Fence(),
            };

            m_renderTargets.push_back(std::move(rt));
        }

        // Create sync objects
        // Create fences in the signaled state
        vk::FenceCreateInfo fenceInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_frames.push_back({
                .imageAvailable = m_pDevice->GetVkDevice().createSemaphoreUnique({}, nullptr),
                .renderFinished = m_pDevice->GetVkDevice().createSemaphoreUnique({}, nullptr),
                .inFlight = m_pDevice->GetVkDevice().createFenceUnique(fenceInfo, nullptr),
            });
        }

        CreatePipelines();
    }

    void CreatePipelines()
    {
        // Create the object rendering pipeline
        pipelines.objects = Pipeline(m_pDevice);
        pipelines.objects.SetRenderPass(m_renderpass.GetVkRenderPass());
        pipelines.objects.SetExtent({m_width, m_height});
        pipelines.objects.RegisterShader("shaders/shader.vert", vk::ShaderStageFlagBits::eVertex);
        pipelines.objects.RegisterShader("shaders/shader.frag", vk::ShaderStageFlagBits::eFragment);
        pipelines.objects.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<Light>());
        pipelines.objects.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<SceneObject>());
        pipelines.objects.Create("objects_pipeline_cache_data.bin");
        m_pDevice->SetDebugUtilsObjectName(pipelines.objects.GetVkPipeline(), "Objects Pipeline");

        // Skybox pipeline
        pipelines.skybox = Pipeline(m_pDevice);
        pipelines.skybox.SetRenderPass(m_renderpass.GetVkRenderPass());
        pipelines.skybox.SetExtent({m_width, m_height});
        pipelines.skybox.RegisterShader("shaders/skybox.vert", vk::ShaderStageFlagBits::eVertex);
        pipelines.skybox.RegisterShader("shaders/skybox.frag", vk::ShaderStageFlagBits::eFragment);
        pipelines.skybox.SetDepthTestWriteEnable(true, false);
        pipelines.skybox.SetRasterizerCullMode(vk::CullModeFlagBits::eNone);
        pipelines.skybox.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<Skybox>());
        pipelines.skybox.Create("skybox_pipeline_cache_data.bin");
        m_pDevice->SetDebugUtilsObjectName(pipelines.skybox.GetVkPipeline(), "Skybox Pipeline");
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
    void BeginFrame()
    {
        // TODO: Handle VK_TIMEOUT and VK_OUT_OF_DATE_KHR
        m_pDevice->GetVkDevice().waitForFences(m_frames[m_currentFrameIndex].inFlight.get(), VK_TRUE, UINT64_MAX);

        // Acquire an image from the swap chain
        auto& image = GetNextTarget();

        // Check if a previous frame is using the image
        if (image.fence)
        {
            m_pDevice->GetVkDevice().waitForFences(image.fence, VK_TRUE, UINT64_MAX);
        }

        // Mark the image as now being in use by this frame
        image.fence = m_frames[m_currentFrameIndex].inFlight.get();

        // Start recording command on this image
        image.commandBuffer->begin(vk::CommandBufferBeginInfo{});

        // Start a render pass
        // TODO: Pass a RenderTarget
        m_renderpass.Begin(image.commandBuffer.get(), image.framebuffer.get());

        // return m_activeImageIndex;
    }

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

        m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // TODO: Link all drawable items with an interface ?
    // TODO: Get rid of the lights descriptor set ref.
    void Draw(std::vector<std::shared_ptr<SceneObject>> objects, vk::UniqueDescriptorSet& lightsDescriptorSet)
    {
        auto cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();
        // Objects
        pipelines.objects.Bind(cb);
        pipelines.objects.BindDescriptorSet(cb, lightsDescriptorSet.get(), 0);
        for (auto model : objects)
        {
            // TODO: Decouple this.
            auto mesh = model->getComponent<Mesh>();

            mesh->Bind(cb);
            pipelines.objects.BindDescriptorSet(cb, model->GetDescriptorSet(), 1);
            cb.drawIndexed(mesh->indices.size(), 1, 0, 0, 0);
        }
    }

    void Draw(std::shared_ptr<Skybox> skybox)
    {
        auto cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();

        // Skybox
        pipelines.skybox.Bind(cb);
        skybox->mesh.Bind(cb);
        pipelines.skybox.BindDescriptorSet(cb, skybox->GetDescriptorSet(), 0);
        cb.drawIndexed(skybox->mesh.indices.size(), 1, 0, 0, 0);
    }

    std::shared_ptr<Device> GetDevice()
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

    uint32_t GetNumberOfImages()
    {
        return m_targetImages.size();
    }

    // TODO Combine Image in RenderTarget
    RenderTarget& GetActiveRenderTarget()
    {
        return m_renderTargets[m_activeImageIndex];
    }

    std::shared_ptr<vkg::Image> GetActiveImage()
    {
        return m_targetImages[m_activeImageIndex];
    }
};
} // namespace vkg