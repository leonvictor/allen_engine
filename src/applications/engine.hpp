#pragma once

#include <anim/module/module.hpp>
#include <assets/module/module.hpp>
#include <core/module/module.hpp>
#include <editor/module/module.hpp>
#include <entities/module/module.hpp>

#include <core/renderers/scene_renderer.hpp>
#include <core/renderers/ui_renderer.hpp>
#include <core/world_systems/render_system.hpp>

#include <assets/asset_service.hpp>
#include <core/services/time_service.hpp>
#include <input/input_service.hpp>
#include <reflection/services/type_registry_service.hpp>
#include <common/services/service_provider.hpp>

#include <entities/world_entity.hpp>
#include <entities/world_update.hpp>

#include <core/asset_loaders/animation_graph_loader.hpp>
#include <core/asset_loaders/animation_loader.hpp>
#include <core/asset_loaders/material_loader.hpp>
#include <core/asset_loaders/mesh_loader.hpp>
#include <core/asset_loaders/skeleton_loader.hpp>
#include <core/asset_loaders/texture_loader.hpp>

#include <editor/editor.hpp>

class GLFWwindow;

namespace aln
{

class Engine
{
  private:
    // Rendering
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
    vkg::ImGUI m_imgui;

    WorldEntity m_worldEntity;
    UpdateContext m_updateContext;

    // Modules
    Assets::Module m_assetsModule;
    Core::Module m_coreModule;
    Anim::Module m_animModule;
    Tooling::Module m_toolingModule;
    Entities::Module m_entitiesModule;

  public:
    Engine() : m_assetService(&m_taskService), m_editor(m_worldEntity) {}

    // TODO: Get rid of the glfwWindow
    void Initialize(GLFWwindow* pGLFWWindow, vkg::Swapchain& swapchain, vkg::Device& device, const Vec2& windowSize)
    {
        // Initialize the render engine
        m_uiRenderer.Create(&swapchain);

        m_sceneRenderer.Initialize(
            &device,
            windowSize.x, windowSize.y,
            2,
            swapchain.GetImageFormat());

        m_imgui.Initialize(pGLFWWindow, &device, m_uiRenderer.GetRenderPass(), m_uiRenderer.GetFramesInFlightCount());

        // TODO: Get rid of all the references to m_device
        // They should not be part of this class
        m_serviceProvider.RegisterService(&m_taskService);
        m_serviceProvider.RegisterService(&m_assetService);
        m_serviceProvider.RegisterService(&m_timeService);
        m_serviceProvider.RegisterService(&m_inputService);
        m_serviceProvider.RegisterService(&m_typeRegistryService);

        m_updateContext.m_pServiceProvider = &m_serviceProvider;

        // Initialize modules
        EngineModuleContext moduleContext;
        moduleContext.m_pTypeRegistryService = &m_typeRegistryService;

        m_coreModule.Initialize(moduleContext);
        m_assetsModule.Initialize(moduleContext);
        m_animModule.Initialize(moduleContext);
        m_toolingModule.Initialize(moduleContext);
        m_entitiesModule.Initialize(moduleContext);

        // TODO: Add a vector of loaded types to the Loader base class, specify them in the constructor of the specialized Loaders,
        // then register each of them with a single function.
        m_assetService.RegisterAssetLoader<StaticMesh, MeshLoader>(&device);
        m_assetService.RegisterAssetLoader<SkeletalMesh, MeshLoader>(&device);
        m_assetService.RegisterAssetLoader<Texture, TextureLoader>(&device);
        m_assetService.RegisterAssetLoader<Material, MaterialLoader>(&device);
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
        // TODO
        m_editor.Shutdown();

        m_sceneRenderer.Shutdown();
    }

    /// @brief Copy the main ImGui context from the Engine class to other DLLs that might need it.
    void ShareImGuiContext()
    {
        EditorImGuiContext context;
        context.m_pImGuiContext = ImGui::GetCurrentContext();
        context.m_pImNodesContext = ImNodes::GetCurrentContext();

        ImGui::GetAllocatorFunctions(&context.m_pAllocFunc, &context.m_pFreeFunc, &context.m_pUserData);

        editor::SetImGuiContext(context);
    }

    void CreateWorld()
    {

    }

    void Update()
    {
        // Update services
        m_inputService.Update();
        m_timeService.Update();
        m_assetService.Update();

        // TODO: Uniformize NewFrame and BeginFrame methods
        m_uiRenderer.BeginFrame(aln::vkg::render::RenderContext());
        m_imgui.NewFrame();

        // Populate update context
        m_updateContext.m_deltaTime = m_timeService.GetDeltaTime();

        auto& dim = m_editor.GetScenePreviewSize();
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
            m_updateContext.m_updateStage = static_cast<UpdateStage>(stage);
            m_worldEntity.Update(m_updateContext);
        }

        auto desc = m_sceneRenderer.GetActiveImage()->GetDescriptorSet();
        m_editor.Update(desc, m_updateContext);

        m_imgui.Render(m_uiRenderer.GetActiveRenderTarget().commandBuffer.get());

        m_uiRenderer.EndFrame();
        m_inputService.ClearFrameState();
    }

    InputService& GetInputService() { return m_inputService; }
};
} // namespace aln