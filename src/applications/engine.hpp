#pragma once

#include <anim/module/module.hpp>
#include <assets/module/module.hpp>
#include <core/module/module.hpp>
#include <editor/module/module.hpp>
#include <entities/module/module.hpp>

#include <assets/asset_service.hpp>
#include <common/services/service_provider.hpp>
#include <core/asset_loaders/animation_graph_loader.hpp>
#include <core/asset_loaders/animation_loader.hpp>
#include <core/asset_loaders/material_loader.hpp>
#include <core/asset_loaders/mesh_loader.hpp>
#include <core/asset_loaders/skeleton_loader.hpp>
#include <core/asset_loaders/texture_loader.hpp>
#include <core/renderers/scene_renderer.hpp>
#include <core/renderers/ui_renderer.hpp>
#include <core/services/time_service.hpp>
#include <core/world_systems/render_system.hpp>
#include <entities/world_entity.hpp>
#include <entities/world_update.hpp>
#include <input/input_service.hpp>
#include <reflection/services/type_registry_service.hpp>

#include <editor/editor.hpp>

#include <tracy/Tracy.hpp>

class GLFWwindow;

namespace aln
{

class Engine
{
    friend class GLFWApplication;

  private:
    // Rendering
    RenderEngine m_renderEngine;
    UIRenderer m_uiRenderer;
    SceneRenderer m_sceneRenderer;

    // Services
    ServiceProvider m_serviceProvider;
    TaskService m_taskService;
    AssetService m_assetService;
    TimeService m_timeService;
    InputService m_inputService;
    TypeRegistryService m_typeRegistryService;

    // Editor
    Editor m_editor;
    ImGUI m_imgui;

    WorldEntity m_worldEntity;
    UpdateContext m_updateContext;

    // Modules
    Assets::Module m_assetsModule;
    Core::Module m_coreModule;
    Anim::Module m_animModule;
    Tooling::Module m_toolingModule;
    Entities::Module m_entitiesModule;

  public:
    Engine() : m_editor(m_worldEntity) {}

    // TODO: Get rid of the glfwWindow
    // void Initialize(GLFWwindow* pGLFWWindow, Swapchain& swapchain, RenderEngine& pRenderEngine, const Vec2& windowSize) void Initialize(GLFWwindow* pGLFWWindow, Swapchain& swapchain, RenderEngine& pRenderEngine, const Vec2& windowSize) void Initialize(GLFWwindow* pGLFWWindow, Swapchain& swapchain, RenderEngine& pRenderEngine, const Vec2& windowSize)
    void Initialize(GLFWwindow* pGLFWWindow, const Vec2& windowSize)
    {
        ZoneScoped;

        // Initialize the render engine
        m_renderEngine.Initialize(pGLFWWindow);
        m_uiRenderer.Initialize(&m_renderEngine.GetSwapchain());
        m_sceneRenderer.Initialize(
            &m_renderEngine,
            windowSize.x, windowSize.y,
            2,
            m_renderEngine.GetSwapchain().GetImageFormat());

        m_imgui.Initialize(pGLFWWindow, &m_renderEngine, m_uiRenderer.GetRenderPass(), m_uiRenderer.GetFramesInFlightCount());

        // Initialize services and provider
        // TODO: Uniformize services initialization. Make it happen in the register function ?
        m_assetService.Initialize(m_taskService, m_renderEngine);
        
        m_serviceProvider.RegisterService(&m_taskService);
        m_serviceProvider.RegisterService(&m_assetService);
        m_serviceProvider.RegisterService(&m_timeService);
        m_serviceProvider.RegisterService(&m_inputService);
        m_serviceProvider.RegisterService(&m_typeRegistryService);

        m_updateContext.m_pServiceProvider = &m_serviceProvider;

        // Initialize modules
        EngineModuleContext moduleContext = {
            .m_pTypeRegistryService = &m_typeRegistryService,
        };

        m_coreModule.Initialize(moduleContext);
        m_assetsModule.Initialize(moduleContext);
        m_animModule.Initialize(moduleContext);
        m_toolingModule.Initialize(moduleContext);
        m_entitiesModule.Initialize(moduleContext);

        // TODO: Add a vector of loaded types to the Loader base class, specify them in the constructor of the specialized Loaders,
        // then register each of them with a single function.
        m_assetService.RegisterAssetLoader<StaticMesh, MeshLoader>(&m_renderEngine);
        m_assetService.RegisterAssetLoader<SkeletalMesh, MeshLoader>(&m_renderEngine);
        m_assetService.RegisterAssetLoader<Texture, TextureLoader>(&m_renderEngine);
        m_assetService.RegisterAssetLoader<Material, MaterialLoader>(&m_renderEngine);
        m_assetService.RegisterAssetLoader<AnimationClip, AnimationLoader>(nullptr);
        m_assetService.RegisterAssetLoader<Skeleton, SkeletonLoader>();
        m_assetService.RegisterAssetLoader<AnimationGraphDataset, AnimationGraphDatasetLoader>();
        m_assetService.RegisterAssetLoader<AnimationGraphDefinition, AnimationGraphDefinitionLoader>(&m_typeRegistryService);

        m_worldEntity.Initialize(m_serviceProvider);
        m_worldEntity.CreateSystem<GraphicsSystem>(&m_sceneRenderer);

        m_editor.Initialize(m_serviceProvider, "scene.aln");

        ShareImGuiContext();
    }

    void Shutdown()
    {
        ZoneScoped;

        m_renderEngine.WaitIdle();

        m_editor.Shutdown();

        // TODO: Destroy world entity
        m_worldEntity.Shutdown();

        m_assetService.Shutdown();


        EngineModuleContext moduleContext = {
            .m_pTypeRegistryService = &m_typeRegistryService,
        };

        m_coreModule.Shutdown(moduleContext);
        m_assetsModule.Shutdown(moduleContext);
        m_animModule.Shutdown(moduleContext);
        m_toolingModule.Shutdown(moduleContext);
        m_entitiesModule.Shutdown(moduleContext);

        m_serviceProvider.UnregisterAllServices();

        m_imgui.Shutdown();

        m_sceneRenderer.Shutdown();
        m_uiRenderer.Shutdown();
        m_renderEngine.Shutdown();
    }

    /// @brief Copy the main ImGui context from the Engine class to other DLLs that might need it.
    void ShareImGuiContext()
    {
        EditorImGuiContext context = {
            .m_pImGuiContext = ImGui::GetCurrentContext(),
            .m_pImNodesContext = ImNodes::GetCurrentContext(),
        };

        ImGui::GetAllocatorFunctions(&context.m_pAllocFunc, &context.m_pFreeFunc, &context.m_pUserData);

        editor::SetImGuiContext(context);
    }

    void Update()
    {
        ZoneScoped;

        // Update services
        m_inputService.Update();
        m_timeService.Update();
        m_assetService.Update();

        // TODO: Uniformize NewFrame and BeginFrame methods
        m_renderEngine.StartFrame();
        m_uiRenderer.StartFrame(aln::render::RenderContext());
        m_imgui.StartFrame();

        // Populate update context
        auto& dim = m_editor.GetScenePreviewSize();
        m_updateContext.m_deltaTime = m_timeService.GetDeltaTime();
        m_updateContext.m_currentTime = m_timeService.GetTime();
        m_updateContext.m_timeSinceAppStart = m_timeService.GetTimeSinceAppStart();
        m_updateContext.m_displayWidth = dim.x;
        m_updateContext.m_displayHeight = dim.y;

        // When out of editor
        // context.displayWidth = m_window.GetWidth();
        // context.displayHeight = m_window.GetHeight();

        // Loading stage
        m_worldEntity.UpdateLoading();

        // Object model: Update systems at various points in the frame.
        // TODO: Handle sync points here ?
        for (uint8_t stage = (uint8_t) UpdateStage::FrameStart; stage != (uint8_t) UpdateStage::NumStages; stage++)
        {
            ZoneScoped;

            m_updateContext.m_updateStage = static_cast<UpdateStage>(stage);
            m_worldEntity.Update(m_updateContext);
        }

        auto desc = m_sceneRenderer.GetActiveImage()->GetDescriptorSet();
        m_editor.Update(desc, m_updateContext);

        m_imgui.EndFrame(m_uiRenderer.GetActiveRenderTarget().commandBuffer.get());
        m_uiRenderer.EndFrame();
        m_renderEngine.EndFrame();

        m_inputService.ClearFrameState();
    }

    InputService& GetInputService() { return m_inputService; }
};
} // namespace aln