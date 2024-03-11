#pragma once

#include "../renderers/scene_renderer.hpp"
#include "../renderers/ui_renderer.hpp"

#include <common/services/service.hpp>
#include <entities/services/worlds_service.hpp>
#include <entities/world_entity.hpp>
#include <graphics/render_engine.hpp>

namespace aln
{
class RenderingService : public IService
{
    RenderEngine* m_pRenderEngine = nullptr;
    ImGUIService* m_pImguiService = nullptr;
    WorldsService* m_pWorldsService = nullptr;

    EditorRenderer m_editorRenderer;
    SceneRenderer m_sceneRenderer;

    RenderContext m_context;

  public:
    void Initialize(ServiceProvider* pProvider) override
    {
        IService::Initialize(pProvider);

        m_pRenderEngine = pProvider->GetRenderEngine();
        m_pWorldsService = pProvider->GetService<WorldsService>();
        assert(m_pWorldsService != nullptr);

        m_editorRenderer.Initialize(m_pRenderEngine);
        m_sceneRenderer.Initialize(m_pRenderEngine);

        m_pImguiService = pProvider->GetService<ImGUIService>();
        if (m_pImguiService != nullptr)
        {
            m_pImguiService->InitializeRendering(m_pRenderEngine, m_editorRenderer.GetRenderPass());
        }
    }

    void Shutdown()
    {
        m_pImguiService->ShutdownRendering();
        m_sceneRenderer.Shutdown();
        m_editorRenderer.Shutdown();
    }

    void Render()
    {
        assert(m_pRenderEngine != nullptr);

        m_pRenderEngine->StartFrame();
        auto cb = m_pRenderEngine->GetGraphicsTransientCommandPool().GetCommandBuffer();

        // Render all worlds
        for (const auto* pWorld : m_pWorldsService->GetWorlds())
        {
            m_sceneRenderer.Render(pWorld, cb);
        }

        // Editor / UI
        m_editorRenderer.StartFrame(cb, m_context);
        m_pImguiService->Render(cb);
        m_editorRenderer.EndFrame(cb);

        QueueSubmissionRequest request;
        request.ExecuteCommandBuffer(cb);
        request.SignalSemaphore(m_editorRenderer.GetCurrentRenderTarget().m_renderFinished);
        request.WaitSemaphore(m_pRenderEngine->GetWindow()->GetSwapchain().GetFrameImageAvailableSemaphore(), 0, vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        m_pRenderEngine->GetGraphicsQueue().Submit(request, m_pRenderEngine->GetCurrentFrameRenderingFence());
        m_pRenderEngine->GetWindow()->GetSwapchain().Present(m_editorRenderer.GetCurrentRenderTarget().m_renderFinished);
        m_pRenderEngine->EndFrame();
    }

    void AcquireWorldGPUResources(Vector<GraphicsSystem::GPUResources>& resources)
    {
        auto colorImageFormat = m_pRenderEngine->GetWindow()->GetSwapchain().GetImageFormat();
        // TODO: Update the viewport based on the imgui scene window size
        auto windowSize = m_pRenderEngine->GetWindow()->GetFramebufferSize();

        auto cb = m_pRenderEngine->GetGraphicsPersistentCommandPool().GetCommandBuffer();

        constexpr auto frameCount = RenderEngine::GetFrameQueueSize();
        resources.reserve(frameCount);
        for (auto renderTargetIdx = 0; renderTargetIdx < frameCount; ++renderTargetIdx)
        {
            auto& frameResources = resources.emplace_back();

            // Resolve image
            frameResources.m_resolveImage.Initialize(
                m_pRenderEngine,
                windowSize.width,
                windowSize.height,
                1,
                vk::SampleCountFlagBits::e1,
                colorImageFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled);
            frameResources.m_resolveImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            frameResources.m_resolveImage.AddView(vk::ImageAspectFlagBits::eColor);
            frameResources.m_resolveImage.AddSampler();
            frameResources.m_resolveImage.TransitionLayout((vk::CommandBuffer) cb, vk::ImageLayout::eGeneral);
            frameResources.m_resolveImage.CreateDescriptorSet();
            frameResources.m_resolveImage.SetDebugName("Scene Renderer Target (" + std::to_string(renderTargetIdx) + ") - Resolve");

            // Multisampling image
            frameResources.m_multisamplingImage.Initialize(
                m_pRenderEngine,
                windowSize.width,
                windowSize.height,
                1,
                m_pRenderEngine->GetMSAASamples(),
                colorImageFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment);

            frameResources.m_multisamplingImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            frameResources.m_multisamplingImage.AddView(vk::ImageAspectFlagBits::eColor);
            frameResources.m_multisamplingImage.SetDebugName("Scene Renderer Target (" + std::to_string(renderTargetIdx) + ") - Multisampling");

            // Depth image
            frameResources.m_depthImage.Initialize(
                m_pRenderEngine,
                windowSize.width,
                windowSize.height,
                1,
                m_pRenderEngine->GetMSAASamples(),
                m_pRenderEngine->FindDepthFormat(),
                vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment);
            frameResources.m_depthImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            frameResources.m_depthImage.AddView(vk::ImageAspectFlagBits::eDepth);
            frameResources.m_depthImage.SetDebugName("Scene Renderer Target (" + std::to_string(renderTargetIdx) + ") - Depth");

            // Framebuffer
            Vector<vk::ImageView> attachments = {
                frameResources.m_multisamplingImage.GetVkView(),
                frameResources.m_depthImage.GetVkView(),
                frameResources.m_resolveImage.GetVkView(),
            };

            vk::FramebufferCreateInfo framebufferInfo = {
                .renderPass = m_sceneRenderer.GetRenderPass().GetVkRenderPass(),
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = windowSize.width,
                .height = windowSize.height,
                .layers = 1,
            };

            frameResources.m_framebuffer = m_pRenderEngine->GetVkDevice().createFramebuffer(framebufferInfo).value;
        }

        QueueSubmissionRequest request;
        request.ExecuteCommandBuffer(cb);

        m_pRenderEngine->GetGraphicsQueue().Submit(request, vk::Fence());
    }

    void ReleaseWorldGPUResources(Vector<GraphicsSystem::GPUResources>& resources)
    {
        constexpr auto frameCount = RenderEngine::GetFrameQueueSize();
        for (auto frameIdx = 0; frameIdx < frameCount; ++frameIdx)
        {
            // Make sure the images are no longer in used before deleting the resources.
            // TODO: This is not good, we're waiting for a full cycle of the frame queue from the main thread
            const auto& fence = m_pRenderEngine->GetFrameRenderingFence(frameIdx);
            m_pRenderEngine->GetVkDevice().waitForFences(1, &fence, vk::True, UINT64_MAX);
        
            auto& frameResources = resources[frameIdx];
            m_pRenderEngine->GetVkDevice().destroyFramebuffer(frameResources.m_framebuffer);
            frameResources.m_depthImage.Shutdown();
            frameResources.m_multisamplingImage.Shutdown();
            frameResources.m_resolveImage.Shutdown();
        }
        resources.clear();
    }

    // Client methods
    // TODO: This should be per-viewport
    void SetBackgroundColor(const RGBAColor& color) { m_context.backgroundColor = color; }
    RenderEngine* GetRenderEngine() const { return m_pRenderEngine; }
};
} // namespace aln