#pragma once

#include "../renderers/scene_renderer.hpp"
#include "../renderers/ui_renderer.hpp"

#include <common/services/service.hpp>
#include <entities/world_entity.hpp>
#include <graphics/render_engine.hpp>

namespace aln
{
class RenderingService : public IService
{
    RenderEngine* m_pRenderEngine = nullptr;
    // TODO: handle multiple worlds (for editor previews)
    WorldEntity* m_pWorld = nullptr;
    ImGUIService* m_pImguiService = nullptr;

    EditorRenderer m_editorRenderer;
    SceneRenderer m_sceneRenderer;

    RenderContext m_context;

  public:
    void Initialize(RenderEngine* pRenderEngine, WorldEntity* pWorld, ImGUIService* pImguiService)
    {
        assert(pRenderEngine != nullptr && pWorld != nullptr);

        m_pRenderEngine = pRenderEngine;
        m_pWorld = pWorld;
        m_pImguiService = pImguiService;

        // Initialize world's viewport
        m_pWorld->InitializeViewport(m_pRenderEngine->GetWindow()->GetFramebufferSize());

        m_editorRenderer.Initialize(m_pRenderEngine);
        m_sceneRenderer.Initialize(m_pRenderEngine);

        m_pImguiService->InitializeRendering(m_pRenderEngine, m_editorRenderer.GetRenderPass());
    }

    void Shutdown()
    {
        m_pImguiService->ShutdownRendering();
        m_sceneRenderer.Shutdown();
        m_editorRenderer.Shutdown();
    }

    void Render()
    {
        assert(m_pRenderEngine != nullptr && m_pWorld != nullptr);

        m_pRenderEngine->StartFrame();
        auto cb = m_pRenderEngine->GetGraphicsTransientCommandPool().GetCommandBuffer();
        
        // Scene
        m_sceneRenderer.Render(m_pWorld, cb);

        // Editor / UI
        m_editorRenderer.StartFrame(cb, m_context);
        m_pImguiService->Render(cb);
        m_editorRenderer.EndFrame(cb);

        Queue::SubmissionRequest request;
        request.ExecuteCommandBuffer(cb);
        request.SignalSemaphore(m_editorRenderer.GetCurrentRenderTarget().m_renderFinished.get());
        request.WaitSemaphore(m_pRenderEngine->GetWindow()->GetSwapchain().GetFrameImageAvailableSemaphore(), vk::PipelineStageFlagBits2::eColorAttachmentOutput);
        
        m_pRenderEngine->GetGraphicsQueue().Submit(request, m_pRenderEngine->GetCurrentFrameRenderingFence());
        m_pRenderEngine->GetWindow()->GetSwapchain().Present(m_editorRenderer.GetCurrentRenderTarget().m_renderFinished.get());
        m_pRenderEngine->EndFrame();
    }

    // Client methods
    // TODO: This should be per-viewport
    void SetBackgroundColor(const RGBAColor& color) { m_context.backgroundColor = color; }
    RenderEngine* GetRenderEngine() const { return m_pRenderEngine; }
    const RenderTarget* GetRenderTarget() const { return m_sceneRenderer.GetCurrentRenderTarget(); }
};
} // namespace aln