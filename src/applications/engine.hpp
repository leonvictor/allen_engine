#pragma once

#include <anim/module/module.hpp>
#include <assets/module/module.hpp>
#include <core/module/module.hpp>
#include <editor/module/module.hpp>
#include <entities/module/module.hpp>

#include <assets/asset_service.hpp>
#include <common/services/service_provider.hpp>

#include <assets/asset_service.hpp>
#include <common/threading/task_service.hpp>
#include <core/services/rendering_service.hpp>
#include <core/services/time_service.hpp>
#include <input/input_service.hpp>
#include <reflection/services/type_registry_service.hpp>

#include <core/renderers/scene_renderer.hpp>
#include <core/renderers/ui_renderer.hpp>
#include <graphics/render_engine.hpp>
#include <graphics/window.hpp>

#include <core/asset_loaders/animation_graph_loader.hpp>
#include <core/asset_loaders/animation_loader.hpp>
#include <core/asset_loaders/material_loader.hpp>
#include <core/asset_loaders/mesh_loader.hpp>
#include <core/asset_loaders/skeleton_loader.hpp>
#include <core/asset_loaders/texture_loader.hpp>

#include <core/world_systems/render_system.hpp>
#include <entities/world_entity.hpp>
#include <entities/world_update.hpp>

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

    // Services
    ServiceProvider m_serviceProvider;
    TaskService* m_pTaskService = nullptr;
    AssetService* m_pAssetService = nullptr;
    TimeService* m_pTimeService = nullptr;
    InputService* m_pInputService = nullptr;
    TypeRegistryService* m_pTypeRegistryService = nullptr;
    RenderingService* m_pRenderingService = nullptr;
    ImGUIService* m_pImguiService = nullptr;

    // Editor
    Editor m_editor;
    ImGUIService m_imguiService;

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
    void Initialize(IWindow* pWindow)
    {
        ZoneScoped;

        // Initialize the render engine
        m_renderEngine.Initialize(pWindow);

        // Initialize services and provider
        m_serviceProvider.Initialize(&m_renderEngine);

        m_serviceProvider.RegisterService(&m_taskService);
        m_serviceProvider.RegisterService(&m_assetService);
        m_pTaskService = m_serviceProvider.AddService<TaskService>();
        m_pAssetService = m_serviceProvider.AddService<AssetService>();
        m_pTimeService = m_serviceProvider.AddService<TimeService>();
        m_pInputService = m_serviceProvider.AddService<InputService>();
        m_pTypeRegistryService = m_serviceProvider.AddService<TypeRegistryService>();
        m_pImguiService = m_serviceProvider.AddService<ImGUIService>();
        m_pRenderingService = m_serviceProvider.AddService<RenderingService>();

        m_updateContext.m_pServiceProvider = &m_serviceProvider;

        // Initialize modules
        EngineModuleContext moduleContext = {
            .m_pTypeRegistryService = m_pTypeRegistryService,
        };

        m_coreModule.Initialize(moduleContext);
        m_assetsModule.Initialize(moduleContext);
        m_animModule.Initialize(moduleContext);
        m_toolingModule.Initialize(moduleContext);
        m_entitiesModule.Initialize(moduleContext);

        // TODO: Add a vector of loaded types to the Loader base class, specify them in the constructor of the specialized Loaders,
        // then register each of them with a single function.
        m_pAssetService->RegisterAssetLoader<StaticMesh, MeshLoader>(&m_renderEngine);
        m_pAssetService->RegisterAssetLoader<SkeletalMesh, MeshLoader>(&m_renderEngine);
        m_pAssetService->RegisterAssetLoader<Texture, TextureLoader>(&m_renderEngine);
        m_pAssetService->RegisterAssetLoader<Material, MaterialLoader>(&m_renderEngine);
        m_pAssetService->RegisterAssetLoader<AnimationClip, AnimationLoader>(nullptr);
        m_pAssetService->RegisterAssetLoader<Skeleton, SkeletonLoader>();
        m_pAssetService->RegisterAssetLoader<AnimationGraphDataset, AnimationGraphDatasetLoader>();
        m_pAssetService->RegisterAssetLoader<AnimationGraphDefinition, AnimationGraphDefinitionLoader>(m_pTypeRegistryService);

        m_worldEntity.Initialize(m_serviceProvider);
        m_worldEntity.CreateSystem<GraphicsSystem>();

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


        EngineModuleContext moduleContext = {
            .m_pTypeRegistryService = m_pTypeRegistryService,
        };

        m_coreModule.Shutdown(moduleContext);
        m_assetsModule.Shutdown(moduleContext);
        m_animModule.Shutdown(moduleContext);
        m_toolingModule.Shutdown(moduleContext);
        m_entitiesModule.Shutdown(moduleContext);

        m_serviceProvider.UnregisterAllServices();
        m_serviceProvider.RemoveService<RenderingService>();
        m_serviceProvider.RemoveService<ImGUIService>();
        m_serviceProvider.RemoveService<TypeRegistryService>();
        m_serviceProvider.RemoveService<InputService>();
        m_serviceProvider.RemoveService<TimeService>();
        m_serviceProvider.RemoveService<AssetService>();
        m_serviceProvider.RemoveService<TaskService>();

        m_serviceProvider.Shutdown();

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
        m_pInputService->Update();
        m_pTimeService->Update();
        m_pAssetService->Update();

        // Populate update context
        auto& dim = m_editor.GetScenePreviewSize();
        m_updateContext.m_deltaTime = m_pTimeService->GetDeltaTime();
        m_updateContext.m_currentTime = m_pTimeService->GetTime();
        m_updateContext.m_timeSinceAppStart = m_pTimeService->GetTimeSinceAppStart();
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

        m_pImguiService->StartFrame();
        m_editor.Update(m_updateContext);
        m_pImguiService->EndFrame();

        m_pRenderingService->Render();

        m_pInputService->ClearFrameState();
    }

    InputService& GetInputService() { return *m_pInputService; }
};
} // namespace aln