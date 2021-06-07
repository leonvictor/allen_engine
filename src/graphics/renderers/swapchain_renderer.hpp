#pragma once

#include "../../light.cpp"
#include "../../scene_object.cpp"
#include "../../skybox.cpp"

#include "../device.hpp"
#include "../imgui.hpp"
#include "../pipeline.hpp"
#include "../render_pass2.hpp"
#include "../render_target.hpp"
#include "../swapchain.hpp"
#include "../window.hpp"

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
class OfflineRenderer
{
    enum State
    {
        Uninitialized,
        Initialized
    };

  private:
    const int MAX_FRAMES_IN_FLIGHT = 2;

    State m_state = State::Uninitialized;

    std::shared_ptr<Device> m_pDevice;

    uint32_t m_currentFrameIndex = 0; // Frame index
    uint32_t m_activeImageIndex;      // Active image index

    // Sync objects
    std::vector<FrameSync> m_frames;

    // Target images
    std::vector<Image> m_targetImages;
    std::vector<RenderTarget> m_renderTargets;

    Image m_colorImage; // Used as an attachment for multisampling
    Image m_depthImage; // Used as an attachment to resolve image depth.

    RenderPass m_renderpass;
    int m_width, m_height;

    struct
    {
        Pipeline objects;
        Pipeline skybox;
    } pipelines;

  public:
    void Create(std::shared_ptr<Device> pDevice, int nTargetImages)
    {
        // Necessary parameters
        // TODO: Grab them from somewhere
        int width, height, mipLevels; // TODO
        vk::Extent2D m_extent;        // TODO
        vk::Format m_format;

        m_pDevice = pDevice;

        // Create color attachment resources for multisampling
        m_colorImage = vkg::Image(m_pDevice, width, height, 1, m_pDevice->GetMSAASamples(), m_format,
                                  vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  vk::ImageAspectFlagBits::eColor);

        // Create depth attachment ressources
        m_depthImage = vkg::Image(m_pDevice, width, height, 1, m_pDevice->GetMSAASamples(),
                                  m_pDevice->FindDepthFormat(), vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  vk::ImageAspectFlagBits::eDepth);

        // TODO: Create the target images
        // TODO: Take swapchain case into account
        // -> Add a constructor with a vk::Image argument to Image
        for (int i = 0; i < nTargetImages; ++i)
        {
            auto image = vkg::Image(m_pDevice, width, height, 1, vk::SampleCountFlagBits::e1, m_format,
                                    vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                    vk::ImageAspectFlagBits::eColor);
            m_pDevice->GetTransferCommandPool().Execute([&](vk::CommandBuffer cb)
                                                        { image.TransitionLayout(cb, vk::ImageLayout::eColorAttachmentOptimal); });

            m_targetImages.push_back(image);
        }

        m_renderpass = RenderPass(m_pDevice, width, height, m_format);
        m_renderpass.AddColorAttachment(m_format);
        m_renderpass.AddDepthAttachment();
        m_renderpass.AddColorResolveAttachment(m_format);

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

        m_renderTargets.resize(m_targetImages.size());
        auto commandBuffers = m_pDevice->GetGraphicsCommandPool().AllocateCommandBuffersUnique(m_targetImages.size());
        int index = 0;

        for (auto& image : m_targetImages)
        {
            RenderTarget rt;

            std::vector<vk::ImageView> attachments = {
                m_colorImage.GetVkView(),
                m_depthImage.GetVkView(),
                image.GetVkView(),
            };

            vk::FramebufferCreateInfo framebufferInfo;
            framebufferInfo.renderPass = m_renderpass.GetVkRenderPass();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1; // Nb of layers in image array.

            rt.framebuffer = m_pDevice->GetVkDevice().createFramebufferUnique(framebufferInfo);
            rt.commandBuffer = std::move(commandBuffers[index]);
            rt.index = index;
            rt.fence = vk::Fence();

            m_renderTargets[index] = std::move(rt);
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

    // TODO: Rename "framebuffer" to something more descriptive of what it actually is
    // RenderTarget ?
    RenderTarget& GetNextTarget()
    {
        m_activeImageIndex++;
        return m_renderTargets[m_activeImageIndex];
        // TODO: use AcquireNextImageKHR if we're using swapchain images
        // Otherwise handle the frames manually
    }

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
};
} // namespace vkg