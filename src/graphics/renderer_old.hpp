#pragma once

#include "../light.cpp"
#include "../scene_object.cpp"
#include "../skybox.cpp"

#include "device.hpp"
#include "imgui.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "swapchain.hpp"
#include "window.hpp"

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
/// Or various subclasses with similar APIs (OfflineRenderer, SwapchainRenderr)

/// @brief Renderer instance used to draw the scenes and the UI.
class Renderer
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
    Window* m_pWindow;           // Associated window
    Swapchain m_targetSwapchain; // Target swapchain

    uint32_t m_currentFrameIndex = 0; // Frame index
    uint32_t m_activeImageIndex;      // Active swapchain image

    // Sync objects
    std::vector<FrameSync> m_frames;

    RenderPass m_renderpass;

    struct
    {
        Pipeline objects;
        Pipeline skybox;
    } pipelines;

    ImGUI m_imgui;

  public:
    // Renderer() {}

    void Create(vkg::Window* pWindow)
    {
        assert(pWindow->IsInitialized());

        m_pWindow = pWindow;
        m_pDevice = std::make_shared<Device>(m_pWindow->GetSurface());
        m_targetSwapchain = vkg::Swapchain(m_pDevice, &m_pWindow->GetSurface(), m_pWindow->GetWidth(), m_pWindow->GetHeight());

        // TODO: Decide where renderpasses should be kept.
        m_renderpass = RenderPass(m_pDevice, &m_targetSwapchain);

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
        m_imgui.Initialize(m_pWindow->GetGLFWWindow(), m_pDevice, m_renderpass, m_targetSwapchain.NumberOfImages());
    }

    void CreatePipelines()
    {
        // Create the object rendering pipeline
        pipelines.objects = Pipeline(m_pDevice);
        pipelines.objects.SetRenderPass(m_renderpass.GetVkRenderPass());
        pipelines.objects.SetExtent(m_targetSwapchain.GetExtent());
        pipelines.objects.RegisterShader("shaders/shader.vert", vk::ShaderStageFlagBits::eVertex);
        pipelines.objects.RegisterShader("shaders/shader.frag", vk::ShaderStageFlagBits::eFragment);
        pipelines.objects.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<Light>());
        pipelines.objects.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<SceneObject>());
        pipelines.objects.Create("objects_pipeline_cache_data.bin");
        m_pDevice->SetDebugUtilsObjectName(pipelines.objects.GetVkPipeline(), "Objects Pipeline");

        // Skybox pipeline
        pipelines.skybox = Pipeline(m_pDevice);
        pipelines.skybox.SetRenderPass(m_renderpass.GetVkRenderPass());
        pipelines.skybox.SetExtent(m_targetSwapchain.GetExtent());
        pipelines.skybox.RegisterShader("shaders/skybox.vert", vk::ShaderStageFlagBits::eVertex);
        pipelines.skybox.RegisterShader("shaders/skybox.frag", vk::ShaderStageFlagBits::eFragment);
        pipelines.skybox.SetDepthTestWriteEnable(true, false);
        pipelines.skybox.SetRasterizerCullMode(vk::CullModeFlagBits::eNone);
        pipelines.skybox.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<Skybox>());
        pipelines.skybox.Create("skybox_pipeline_cache_data.bin");
        m_pDevice->SetDebugUtilsObjectName(pipelines.skybox.GetVkPipeline(), "Skybox Pipeline");
    }

    void BeginFrame()
    {
        // TODO: Handle VK_TIMEOUT and VK_OUT_OF_DATE_KHR
        m_pDevice->GetVkDevice().waitForFences(m_frames[m_currentFrameIndex].inFlight.get(), VK_TRUE, UINT64_MAX);

        // TODO: Handle recreation
        // Acquire an image from the swap chain
        m_activeImageIndex = m_targetSwapchain.AcquireNextImage(m_frames[m_currentFrameIndex].imageAvailable.get());

        // TODO: If fences and commandbuffers actually deserve to be in swapchain,
        // This could be in swapchain as well

        // Check if a previous frame is using the image
        if (m_targetSwapchain.ActiveImage().fence)
        {
            m_pDevice->GetVkDevice().waitForFences(m_targetSwapchain.ActiveImage().fence, VK_TRUE, UINT64_MAX);
        }

        // Mark the image as now being in use by this frame
        m_targetSwapchain.ActiveImage().fence = m_frames[m_currentFrameIndex].inFlight.get();

        m_targetSwapchain.ActiveImage().commandbuffer->begin(vk::CommandBufferBeginInfo{});

        // Start a render pass
        m_renderpass.Begin(m_targetSwapchain.ActiveImage().commandbuffer.get(), m_activeImageIndex);

        m_imgui.NewFrame();

        // return m_activeImageIndex;
    }

    void EndFrame()
    {
        m_imgui.Render(m_targetSwapchain.ActiveImage().commandbuffer.get());

        // TODO: move out
        m_targetSwapchain.ActiveImage().commandbuffer->endRenderPass();
        m_targetSwapchain.ActiveImage().commandbuffer->end();

        // At which stage should we wait for each semaphores (in the same order)
        // vk::PipelineStageFlagBits waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        // Which semaphores to wait for
        // auto waitSemaphores = {m_imageAvailableSemaphores[m_currentFrameIndex].get()};
        // Which semaphores to signal when job is done
        // vk::UniqueSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrameIndex].get()};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo submitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_frames[m_currentFrameIndex].imageAvailable.get(),
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_targetSwapchain.ActiveImage().commandbuffer.get(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &m_frames[m_currentFrameIndex].renderFinished.get(),
        };

        m_pDevice->GetVkDevice().resetFences(m_frames[m_currentFrameIndex].inFlight.get());
        // TODO: It would be better to pass the semaphores and cbs directly to the queue class
        // but we need a mechanism to avoid having x versions of the method for (single elements * arrays * n_occurences)
        // vulkan arraywrappers ?
        m_pDevice->GetGraphicsQueue()
            .Submit(submitInfo, m_frames[m_currentFrameIndex].inFlight.get());

        vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_frames[m_currentFrameIndex].renderFinished.get(),
            .swapchainCount = 1,
            .pSwapchains = &m_targetSwapchain.m_vkSwapchain.get(),
            .pImageIndices = &m_activeImageIndex,
            .pResults = nullptr, // For checking every individual swap chain results. We only have one so we don't need it
        };

        // TODO: Rework cuz it's ugly (see https://github.com/liblava/liblava/blob/3bce924a014529a9d18cec9a406d3eab6850e159/liblava/frame/renderer.cpp)
        // TODO: Handle swapchain recreation
        bool recreationNeeded = false;
        vk::Result result;
        try
        {
            result = m_pDevice->GetGraphicsQueue().GetVkQueue().presentKHR(presentInfo);
        }
        catch (vk::OutOfDateKHRError const& e)
        {
            result = vk::Result::eErrorOutOfDateKHR;
        }

        // TODO: Shoud this happen in swapchain directly ?
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_pWindow->m_framebufferResized)
        {
            while (m_pWindow->IsMinimized())
            {
                m_pWindow->WaitEvents();
            }

            auto size = m_pWindow->GetSize();
            m_targetSwapchain.Resize(size.width, size.height);
            m_renderpass.Resize(&m_targetSwapchain);
            CreatePipelines();

            m_pWindow->m_framebufferResized = false;
        }
        else if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to present swap chain image.");
        }

        m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // TODO: Link all drawable items with an interface ?
    // TODO: Get rid of the lights descriptor set ref.
    void Draw(std::vector<std::shared_ptr<SceneObject>> objects, vk::UniqueDescriptorSet& lightsDescriptorSet)
    {
        auto cb = m_targetSwapchain.ActiveImage().commandbuffer.get();
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
        auto cb = m_targetSwapchain.ActiveImage().commandbuffer.get();

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

    Swapchain& GetSwapchain()
    {
        // TODO: Get rid of all the references.
        std::cout << "Warning: accessing swapchain outside of renderer context." << std::endl;
        return m_targetSwapchain;
    }

    RenderPass& GetRenderPass()
    {
        // TODO: Get rid of all the references.
        std::cout << "Warning: accessing renderpass outside of renderer context." << std::endl;
        return m_renderpass;
    }

    vk::CommandBuffer& GetActiveImageCommandBuffer()
    {
        return m_targetSwapchain.ActiveImage().commandbuffer.get();
    }
};
} // namespace vkg