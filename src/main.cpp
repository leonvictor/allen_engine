#include <glm/gtc/random.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <cstdlib>
#include <fmt/core.h>
#include <functional>
#include <stdexcept>
#include <string.h>

#include <editor/editor.hpp>

#include <graphics/device.hpp>
#include <graphics/imgui.hpp>
#include <graphics/instance.hpp>
#include <graphics/window.hpp>

#include <core/renderers/scene_renderer.hpp>
#include <core/renderers/ui_renderer.hpp>

#ifdef ALN_DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>

#include <crtdbg.h>
#endif

#include <assets/asset_service.hpp>
#include <core/services/time_service.hpp>
#include <input/input_service.hpp>
#include <reflection/services/type_registry_service.hpp>

#include <entities/entity.hpp>
#include <entities/entity_descriptors.hpp>
#include <entities/world_entity.hpp>
#include <entities/world_update.hpp>

#include <core/skeletal_mesh.hpp>
#include <core/static_mesh.hpp>

#include <core/components/animation_player_component.hpp>
#include <core/components/camera.hpp>
#include <core/components/light.hpp>
#include <core/components/skeletal_mesh_component.hpp>
#include <core/components/static_mesh_component.hpp>

#include <core/entity_systems/animation_system.hpp>
#include <core/entity_systems/camera_controller.hpp>
#include <core/entity_systems/script.hpp>

#include <core/world_systems/render_system.hpp>

#include <anim/module/module.hpp>
#include <assets/module/module.hpp>
#include <core/module/module.hpp>
#include <editor/module/module.hpp>
#include <entities/module/module.hpp>

#include <core/asset_loaders/animation_graph_loader.hpp>
#include <core/asset_loaders/animation_loader.hpp>
#include <core/asset_loaders/material_loader.hpp>
#include <core/asset_loaders/mesh_loader.hpp>
#include <core/asset_loaders/skeleton_loader.hpp>
#include <core/asset_loaders/texture_loader.hpp>

#include <common/memory.hpp>
#include <common/services/service_provider.hpp>
#include <common/threading/task_service.hpp>

#include <anim/animation_clip.hpp>

#include <config/path.h>

#include <Tracy.hpp>
#include <nlohmann/json.hpp>

// Test anim graph
#include <anim/graph/nodes/animation_clip_node.hpp>

namespace aln
{

static constexpr glm::vec3 WORLD_ORIGIN = glm::vec3(0.0f);
static constexpr glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
static constexpr glm::vec3 WORLD_BACKWARD = -WORLD_FORWARD;
static constexpr glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
static constexpr glm::vec3 WORLD_LEFT = -WORLD_RIGHT;
static constexpr glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
static constexpr glm::vec3 WORLD_DOWN = -WORLD_UP;

class Engine
{
  private:
    // Rendering
    vkg::Instance m_instance;
    vkg::Window m_window;
    vkg::Device m_device;
    vkg::Swapchain m_swapchain;

    aln::UIRenderer m_uiRenderer;
    aln::SceneRenderer m_sceneRenderer;

    // Services
    TaskService m_taskService;
    AssetService m_assetService;
    TimeService m_timeService;
    InputService m_inputService;
    TypeRegistryService m_typeRegistryService;

    ServiceProvider m_serviceProvider;

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

    void Run()
    {
        while (!m_window.ShouldClose())
        {
            Update();
            FrameMark;
        }
    }

    void Initialize()
    {
        // Initialize the render engine
        // @todo: Improve API
        m_window.InitializeWindow();
        m_instance.RequestExtensions(m_window.GetRequiredExtensions());
        m_instance.Create();
        m_window.CreateSurface(&m_instance);

        m_device.Initialize(&m_instance, m_window.GetVkSurface());
        m_swapchain = vkg::Swapchain(&m_device, &m_window);

        m_uiRenderer.Create(&m_swapchain);

        m_sceneRenderer.Create(
            &m_device,
            m_window.GetWidth(), m_window.GetHeight(),
            2,
            m_swapchain.GetImageFormat());

        m_imgui.Initialize(m_window.GetGLFWWindow(), &m_device, m_uiRenderer.GetRenderPass(), m_uiRenderer.GetNumberOfImages());

        // Register callbacks to transfer events from the window to the input system
        // TODO: Ideally this would be managed entirelly by the input system, without a dependency on the window
        m_window.AddKeyCallback(std::bind(&InputService::UpdateKeyboardControlState, &m_inputService, std::placeholders::_1, std::placeholders::_2));
        m_window.AddMouseButtonCallback(std::bind(&InputService::UpdateMouseControlState, &m_inputService, std::placeholders::_1, std::placeholders::_2));
        m_window.AddScrollCallback(std::bind(&InputService::UpdateScrollControlState, &m_inputService, std::placeholders::_1, std::placeholders::_2));

        // TODO: Get rid of all the references to m_device
        // They should not be part of this class

        m_serviceProvider.RegisterService(&m_taskService);
        m_serviceProvider.RegisterService(&m_assetService);
        m_serviceProvider.RegisterService(&m_timeService);
        m_serviceProvider.RegisterService(&m_inputService);
        m_serviceProvider.RegisterService(&m_typeRegistryService);

        m_updateContext.m_pServiceProvider = &m_serviceProvider;

        // Initialize modules
        // TODO: Use Initialize fn + context
        // ModuleContext moduleContext;
        // moduleContext.m_pTypeRegistryService = &m_typeRegistryService;
        // ...

        m_coreModule.RegisterTypes(&m_typeRegistryService);
        m_assetsModule.RegisterTypes(&m_typeRegistryService);
        m_animModule.RegisterTypes(&m_typeRegistryService);
        m_toolingModule.RegisterTypes(&m_typeRegistryService);
        m_entitiesModule.RegisterTypes(&m_typeRegistryService);

        // TODO: Add a vector of loaded types to the Loader base class, specify them in the constructor of the specialized Loaders,
        // then register each of them with a single function.
        m_assetService.RegisterAssetLoader<StaticMesh, MeshLoader>(&m_device);
        m_assetService.RegisterAssetLoader<SkeletalMesh, MeshLoader>(&m_device);
        m_assetService.RegisterAssetLoader<Texture, TextureLoader>(&m_device);
        m_assetService.RegisterAssetLoader<Material, MaterialLoader>(&m_device);
        m_assetService.RegisterAssetLoader<AnimationClip, AnimationLoader>(nullptr);
        m_assetService.RegisterAssetLoader<Skeleton, SkeletonLoader>();
        m_assetService.RegisterAssetLoader<AnimationGraphDataset, AnimationGraphDatasetLoader>();
        m_assetService.RegisterAssetLoader<AnimationGraphDefinition, AnimationGraphDefinitionLoader>(&m_typeRegistryService);

        CreateWorld();
        ShareImGuiContext();
    }

    void Shutdown()
    {
        // TODO
        m_device.GetVkDevice().waitIdle();
        m_editor.Shutdown();
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
        m_worldEntity.Initialize(m_serviceProvider);
        m_worldEntity.CreateSystem<GraphicsSystem>(&m_sceneRenderer);

        m_editor.Initialize(m_serviceProvider, "scene.aln");
    }

    void Update()
    {
        // Update services

        // todo: Move to InputService::Update. We need to associate the service with the window beforehand
        m_inputService.UpdateMousePosition(m_window.GetCursorPosition());

        m_inputService.Update();
        m_timeService.Update();
        m_assetService.Update();

        // TODO: Uniformize Update, NewFrame, Dispatch, and BeginFrame methods
        m_window.NewFrame();
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
        for (uint8_t stage = UpdateStage::FrameStart; stage != UpdateStage::NumStages; stage++)
        {
            m_updateContext.m_updateStage = static_cast<UpdateStage>(stage);
            m_worldEntity.Update(m_updateContext);
        }

        auto desc = m_sceneRenderer.GetActiveImage()->GetDescriptorSet();
        m_editor.Update(desc, m_updateContext);

        m_imgui.Render(m_uiRenderer.GetActiveRenderTarget().commandBuffer.get());

        m_uiRenderer.EndFrame();
    }
};
} // namespace aln

int main()
{
#ifdef ALN_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    aln::Engine* pApp = aln::New<aln::Engine>();

    pApp->Initialize();
    pApp->Run();
    pApp->Shutdown();

    aln::Delete(pApp); // Test deletion

    return EXIT_SUCCESS;
};
