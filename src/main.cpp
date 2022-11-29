#include <glm/gtc/random.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <cstdlib>
#include <fmt/core.h>
#include <functional>
#include <stdexcept>
#include <string.h>

#include <editor/editor.hpp>

#include <reflection/reflection.hpp>

#include <graphics/device.hpp>
#include <graphics/imgui.hpp>
#include <graphics/instance.hpp>
#include <graphics/rendering/offline_renderer.hpp>
#include <graphics/rendering/swapchain_renderer.hpp>
#include <graphics/window.hpp>

#ifdef ALN_DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>

#include <crtdbg.h>
#endif

#include <input/input_service.hpp>

#include <entities/entity.hpp>
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

#include <assets/asset_service.hpp>
#include <core/services/time_service.hpp>

#include <core/asset_loaders/animation_loader.hpp>
#include <core/asset_loaders/material_loader.hpp>
#include <core/asset_loaders/mesh_loader.hpp>
#include <core/asset_loaders/skeleton_loader.hpp>
#include <core/asset_loaders/texture_loader.hpp>

#include <common/memory.hpp>
#include <common/services/service_provider.hpp>
#include <common/threading/task_service.hpp>

#include <anim/animation_clip.hpp>

#include "IconsFontAwesome4.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include <config/path.h>

#include <Tracy.hpp>

// Test anim graph
#include <anim/graph/graph.hpp>
#include <anim/graph/nodes/animation_clip_node.hpp>

namespace aln
{

// Mike
// const std::string MODEL_PATH = std::string(DEFAULT_ASSETS_DIR) + "/models/assets_export/Mike/Cube.010.smsh";
// const std::string TEXTURE_PATH = std::string(DEFAULT_ASSETS_DIR) + "/models/assets_export/Mike_Texture.text";
// const std::string TEST_SKELETON_PATH = std::string(DEFAULT_ASSETS_DIR) + "/models/assets_export/Mike/RobotArmature.skel";

// Cesium man
const std::string MODEL_PATH = std::string(DEFAULT_ASSETS_DIR) + "/assets_export/CesiumMan/Cesium_Man.smsh";
const std::string TEXTURE_PATH = std::string(DEFAULT_ASSETS_DIR) + "/assets_export/CesiumMan_img0.text";
const std::string TEST_SKELETON_PATH = std::string(DEFAULT_ASSETS_DIR) + "/assets_export/CesiumMan/Armature.skel";
const std::string MATERIAL_PATH = std::string(DEFAULT_ASSETS_DIR) + "/assets_export/CesiumMan/Cesium_Man-effect.mtrl";

static constexpr glm::vec3 WORLD_ORIGIN = glm::vec3(0.0f);
static constexpr glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
static constexpr glm::vec3 WORLD_BACKWARD = -WORLD_FORWARD;
static constexpr glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
static constexpr glm::vec3 WORLD_LEFT = -WORLD_RIGHT;
static constexpr glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
static constexpr glm::vec3 WORLD_DOWN = -WORLD_UP;

class Engine
{
  public:
    Engine() : m_assetService(&m_taskService), m_editor(m_worldEntity)
    {
        // Initialize the render engine
        // @todo: Improve API
        m_window.InitializeWindow();
        m_instance.RequestExtensions(m_window.GetRequiredExtensions());
        m_instance.Create();
        m_window.CreateSurface(&m_instance);

        m_device.Initialize(&m_instance, m_window.GetVkSurface());
        m_swapchain = vkg::Swapchain(&m_device, &m_window);

        m_renderer.Create(&m_swapchain);

        m_sceneRenderer.Create(
            &m_device,
            m_window.GetWidth(), m_window.GetHeight(),
            2,
            m_swapchain.GetImageFormat());

        m_imgui.Initialize(m_window.GetGLFWWindow(), &m_device, m_renderer.GetRenderPass(), m_renderer.GetNumberOfImages());

        // Register callbacks to transfer events from the window to the input system
        // TODO: Ideally this would be managed entirelly by the input system, without a dependency on the window
        m_window.AddKeyCallback(std::bind(&InputService::UpdateKeyboardControlState, &m_inputService, std::placeholders::_1, std::placeholders::_2));
        m_window.AddMouseButtonCallback(std::bind(&InputService::UpdateMouseControlState, &m_inputService, std::placeholders::_1, std::placeholders::_2));
        m_window.AddScrollCallback(std::bind(&InputService::UpdateScrollControlState, &m_inputService, std::placeholders::_1, std::placeholders::_2));

        // TODO: Get rid of all the references to m_device
        // They should not be part of this class

        // TODO: Add a vector of loaded types to the Loader base class, specify them in the constructor of the specialized Loaders,
        // then register each of them with a single function.
        m_assetService.RegisterAssetLoader<StaticMesh, MeshLoader>(&m_device);
        m_assetService.RegisterAssetLoader<SkeletalMesh, MeshLoader>(&m_device);
        m_assetService.RegisterAssetLoader<Texture, TextureLoader>(&m_device);
        m_assetService.RegisterAssetLoader<Material, MaterialLoader>(&m_device);
        m_assetService.RegisterAssetLoader<AnimationClip, AnimationLoader>(nullptr);
        m_assetService.RegisterAssetLoader<Skeleton, SkeletonLoader>();

        m_serviceProvider.RegisterService(&m_taskService);
        m_serviceProvider.RegisterService(&m_assetService);
        m_serviceProvider.RegisterService(&m_timeService);
        m_serviceProvider.RegisterService(&m_inputService);

        m_updateContext.m_pServiceProvider = &m_serviceProvider;

        CreateWorld();
        ShareImGuiContext();
    }

    void run()
    {
        Update();
    }

  private:
    vkg::Instance m_instance;
    vkg::Window m_window;
    vkg::Device m_device;
    vkg::Swapchain m_swapchain;
    vkg::render::SwapchainRenderer m_renderer;
    vkg::render::OfflineRenderer m_sceneRenderer;

    // Services
    TaskService m_taskService;
    AssetService m_assetService;
    TimeService m_timeService;
    InputService m_inputService;

    ServiceProvider m_serviceProvider;

    editor::Editor m_editor;
    vkg::ImGUI m_imgui;

    WorldEntity m_worldEntity;
    UpdateContext m_updateContext;

    /// @brief Copy the main ImGui context from the Engine class to other DLLs that might need it.
    void ShareImGuiContext()
    {
        editor::EditorImGuiContext context;
        context.m_pImGuiContext = ImGui::GetCurrentContext();
        context.m_pImNodesContext = ImNodes::GetCurrentContext();

        ImGui::GetAllocatorFunctions(&context.m_pAllocFunc, &context.m_pFreeFunc, &context.m_pUserData);

        editor::SetImGuiContext(context);
    }

    void CreateWorld()
    {
        m_worldEntity.Initialize(m_serviceProvider);
        m_worldEntity.CreateSystem<GraphicsSystem>(&m_sceneRenderer);

        // Create some entities
        {
            // TODO: Refine how the editor accesses the map
            Entity* pCameraEntity = m_worldEntity.m_entityMap.CreateEntity("MainCamera");
            auto pCameraComponent = aln::New<Camera>();
            pCameraComponent->SetLocalTransformPosition(glm::vec3(0.0f, 0.0f, -20.0f));
            pCameraEntity->AddComponent(pCameraComponent);

            pCameraEntity->CreateSystem<EditorCameraController>();
        }

        {
            Entity* pLightEntity = m_worldEntity.m_entityMap.CreateEntity("DirectionalLight");
            Light* pLightComponent = aln::New<Light>();
            pLightComponent->direction = WORLD_RIGHT;
            pLightComponent->type = Light::Type::Directional;
            pLightComponent->SetLocalTransformPosition(glm::vec3(-4.5f));
            pLightEntity->AddComponent(pLightComponent);

            Entity* pPointLightEntity = m_worldEntity.m_entityMap.CreateEntity("PointLight");
            Light* pPLightComponent = aln::New<Light>();
            pPLightComponent->direction = WORLD_RIGHT;
            pPLightComponent->type = Light::Type::Point;
            pPLightComponent->SetLocalTransformPosition(glm::vec3(-4.5f));
            pPointLightEntity->AddComponent(pPLightComponent);
        }

        auto pMesh = aln::New<SkeletalMeshComponent>();
        pMesh->SetMesh(MODEL_PATH);
        pMesh->SetSkeleton(TEST_SKELETON_PATH);
        pMesh->SetMaterial(MATERIAL_PATH);
        pMesh->SetRenderDevice(&m_device);

        auto pAnim = aln::New<AnimationPlayerComponent>();
        pAnim->SetSkeleton(std::string(DEFAULT_ASSETS_DIR) + "/assets_export/CesiumMan/Armature.skel");
        pAnim->SetAnimationClip(std::string(DEFAULT_ASSETS_DIR) + "/assets_export/CesiumMan/Default.anim");

        Entity* pCube = m_worldEntity.m_entityMap.CreateEntity("Cesium Man");
        pCube->CreateSystem<AnimationSystem>();
        pCube->AddComponent(pMesh);
        pCube->AddComponent(pAnim);
    }

    void Update()
    {
        while (!m_window.ShouldClose())
        {
            // Update services

            // todo: Move to InputService::Update. We need to associate the service with the window beforehand
            m_inputService.UpdateMousePosition(m_window.GetCursorPosition());

            m_inputService.Update();
            m_timeService.Update();
            m_assetService.Update();

            // TODO: Uniformize Update, NewFrame, Dispatch, and BeginFrame methods
            m_window.NewFrame();
            m_renderer.BeginFrame(aln::vkg::render::RenderContext());
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
            m_editor.DrawUI(desc, m_timeService.GetDeltaTime());

            m_imgui.Render(m_renderer.GetActiveRenderTarget().commandBuffer.get());

            m_renderer.EndFrame();
        }

        m_device.GetVkDevice().waitIdle();

        FrameMark;
    }
};
} // namespace aln

int main()
{
#ifdef ALN_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    std::unique_ptr<aln::Engine> app = std::make_unique<aln::Engine>();
    app->run();

    app.reset();
    return EXIT_SUCCESS;
};
