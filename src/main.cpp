#include <glm/vec3.hpp>

#include <chrono> // std::chrono::seconds
#include <thread> // std::this_thread::sleep_for

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string.h>
#include <unordered_map>
#include <vector>

#include "camera_controller.hpp"

#include <graphics/device.hpp>
#include <graphics/imgui.hpp>
#include <graphics/instance.hpp>
#include <graphics/rendering/offline_renderer.hpp>
#include <graphics/rendering/swapchain_renderer.hpp>
#include <graphics/ubo.hpp>
#include <graphics/window.hpp>

#include "time_system.hpp"
#include <input/input_system.hpp>

#include <entities/entity.hpp>
#include <entities/world_entity.hpp>
#include <entities/world_update.hpp>

#include <core/camera.hpp>
#include <core/light.hpp>
#include <core/mesh_renderer.hpp>
#include <core/render_system.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"

#include <config/path.h>
#include <reflection/reflection.hpp>

namespace aln
{

const std::string MODEL_PATH = std::string(DEFAULT_ASSETS_DIR) + "/models/cube.obj";
const std::string TEXTURE_PATH = std::string(DEFAULT_ASSETS_DIR) + "/textures/container2.png";
const int MAX_MODELS = 50;

class Engine
{
  public:
    Engine()
    {
        m_window.InitializeWindow();
        m_instance.RequestExtensions(m_window.GetRequiredExtensions());
        m_instance.Create();
        m_window.CreateSurface(&m_instance);

        m_pDevice = std::make_shared<vkg::Device>(&m_instance, m_window.GetVkSurface());
        m_swapchain = vkg::Swapchain(m_pDevice, &m_window);

        m_renderer.Create(&m_swapchain);

        m_sceneRenderer.Create(
            m_pDevice,
            m_window.GetWidth(), m_window.GetHeight(),
            2,
            m_swapchain.GetImageFormat());

        m_imgui.Initialize(m_window.GetGLFWWindow(), m_pDevice, m_renderer.GetRenderPass(), m_renderer.GetNumberOfImages());

        // Register callbacks to transfer events from the window to the input system
        // TODO: Ideally this would be managed entirelly by the input system, without a dependency on the window
        m_window.AddKeyCallback(std::bind(Input::UpdateKeyboardControlState, std::placeholders::_1, std::placeholders::_2));
        m_window.AddMouseButtonCallback(std::bind(Input::UpdateMouseControlState, std::placeholders::_1, std::placeholders::_2));
        m_window.AddScrollCallback(std::bind(Input::UpdateScrollControlState, std::placeholders::_1, std::placeholders::_2));

        // TODO: Get rid of all the references to m_pDevice
        // They should not be part of this class

        setupSkyBox();

        // TODO:
        //  - Do not update if no object were modified
        //  - Only update objects which have been modified
        // TODO: Let the scene handle its own descriptions (eg. do not pass each model to the swapchain like this)

        CreateWorld();
        ShareImGuiContext();
    }

    void run()
    {
        mainLoop();
    }

  private:
    vkg::Instance m_instance;
    vkg::Window m_window;
    std::shared_ptr<vkg::Device> m_pDevice;
    vkg::Swapchain m_swapchain;
    vkg::render::SwapchainRenderer m_renderer;
    vkg::render::OfflineRenderer m_sceneRenderer;
    vkg::ImGUI m_imgui;

    const glm::vec3 WORLD_ORIGIN = glm::vec3(0.0f);
    const glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 WORLD_BACKWARD = -WORLD_FORWARD;
    const glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 WORLD_LEFT = -WORLD_RIGHT;
    const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 WORLD_DOWN = -WORLD_UP;

    const glm::vec3 LIGHT_POSITION = glm::vec3(-4.5f);

    std::array<glm::vec3, 3> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 5.0f, -15.0f),
        LIGHT_POSITION};

    // GUI toggles
    bool showTransformGUI = false;
    Entity* m_pSelectedEntity = nullptr;

    // Object model
    WorldEntity m_worldEntity;

    /// @brief Copy the main ImGui context from the Engine class to other DLLs that might need it.
    void ShareImGuiContext()
    {
        ImGuiMemAllocFunc pAllocFunc;
        ImGuiMemFreeFunc pFreeFunc;
        void* pUserData;

        ImGui::GetAllocatorFunctions(&pAllocFunc, &pFreeFunc, &pUserData);

        reflect::SetImGuiContext(ImGui::GetCurrentContext());
        reflect::SetImGuiAllocatorFunctions(&pAllocFunc, &pFreeFunc, &pUserData);
    }

    void CreateWorld()
    {
        m_worldEntity.CreateSystem<GraphicsSystem>(&m_sceneRenderer);

        // Create some entities
        {
            Entity* pCameraEntity = Entity::Create("MainCamera");
            auto pCameraComponent = pCameraEntity->AddComponent<Camera>();

            pCameraComponent->ModifyTransform()->position = WORLD_BACKWARD * 2.0f;
            pCameraComponent->ModifyTransform()->rotation.x = 90.0f;

            pCameraEntity->CreateSystem<EditorCameraController>();
        }

        {
            Entity* pLightEntity = Entity::Create("DirectionnalLight");
            Light* pLightComponent = pLightEntity->AddComponent<Light>();
            pLightComponent->color = glm::vec3(1.0f);
            pLightComponent->direction = WORLD_RIGHT;
            pLightComponent->type = Light::Type::Directionnal;
            auto t = pLightComponent->ModifyTransform()->position = LIGHT_POSITION;

            Entity* pPointLightEntity = Entity::Create("PointLight");
            Light* pPLightComponent = pPointLightEntity->AddComponent<Light>();
            pPLightComponent->color = glm::vec3(1.0f);
            pPLightComponent->direction = WORLD_RIGHT;
            pPLightComponent->type = Light::Type::Directionnal;
            t = pPLightComponent->ModifyTransform()->position = LIGHT_POSITION;
        }

        for (auto pos : cubePositions)
        {
            // TODO: This api is too verbose
            Entity* pCube = Entity::Create("cube");
            auto pMesh = pCube->AddComponent<MeshRenderer>(m_pDevice, MODEL_PATH, TEXTURE_PATH);
            pMesh->ModifyTransform()->position = pos;
        }
    }

    void setupSkyBox()
    {
        // skybox = std::make_shared<Skybox>(m_pDevice, "", MODEL_PATH);
        // updateSkyboxUBO();
    }

    void updateSkyboxUBO()
    {
        // vkg::UniformBufferObject ubo;
        // ubo.model = glm::mat4(glm::mat3(camera.getViewMatrix()));
        // ubo.view = glm::mat4(1.0f);                                                                                                            // eye/camera position, center position, up axis
        // ubo.projection = glm::perspective(glm::radians(45.0f), (float) m_swapchain.GetWidth() / (float) m_swapchain.GetHeight(), 0.1f, 300.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
        // ubo.projection[1][1] *= -1;                                                                                                            // GLM is designed for OpenGL which uses inverted y coordinates
        // ubo.cameraPos = camera.transform.position;
        // skybox->updateUniformBuffer(ubo);
    }

    void mainLoop()
    {
        // TODO: Move to window
        while (!m_window.ShouldClose())
        {
            // std::this_thread::sleep_for(std::chrono::seconds(1));
            // TODO: Uniformize Update, NewFrame, Dispatch, and BeginFrame methods
            Time::Update();

            // TODO: Group glfw accesses in a window.NewFrame() method
            // Map GLFW events to the Input system
            m_window.NewFrame();

            Input::UpdateMousePosition(m_window.GetCursorPosition());
            // Trigger input callbacks
            Input::Dispatch();

            m_renderer.BeginFrame();
            m_imgui.NewFrame();

            // Object model: Update systems at various points in the frame.
            // TODO: Handle sync points here ?
            aln::entities::UpdateContext context = aln::entities::UpdateContext(UpdateStage::FrameStart);
            context.displayWidth = m_window.GetWidth();
            context.displayHeight = m_window.GetHeight();
            m_worldEntity.Update(context);

            context = aln::entities::UpdateContext(UpdateStage::FrameEnd);
            context.displayWidth = m_window.GetWidth();
            context.displayHeight = m_window.GetHeight();
            m_worldEntity.Update(context);

            updateSkyboxUBO();

            // TODO: Handle picker again
            // if (input.isPressedLastFrame(GLFW_MOUSE_BUTTON_LEFT, true) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGui::IsAnyItemHovered())
            // {
            //     auto rgb = swapchain->picker.pickColor(models, lastMousePos);
            //     auto cID = ColorUID(rgb);
            //     // TODO: Handle background
            //     // TODO: Make sure ColorUID has a reserved id for the background
            //     selectedObject = clickables[cID];
            // }

            DrawUI();

            m_imgui.Render(m_renderer.GetActiveRenderTarget().commandBuffer.get());
            m_renderer.EndFrame();
        }

        m_pDevice->GetVkDevice().waitIdle();
    }

    void DrawUI()
    {
        // Draw ImGUI components
        ImGuiViewportP* viewport = (ImGuiViewportP*) (void*) ImGui::GetMainViewport();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        float height = ImGui::GetFrameHeight();

        auto dockID = ImGui::DockSpaceOverViewport(viewport);

        // TODO: Programatically set the initial layout
        if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    ImGui::MenuItem("Item");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("View"))
                {
                    ImGui::MenuItem("Item");
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
        }
        ImGui::End();

        if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                // Compute current FPS
                // Use std::format (C++20). Not available in most compilers as of 04/06/2021
                std::string fps = std::to_string(1.0 / Time::GetDeltaTime());
                fps = fps.substr(0, fps.find("."));
                fps += " FPS";
                ImGui::Text(fps.c_str());

                ImGui::EndMenuBar();
            }
        }
        ImGui::End();

        if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar))
        {
            // TODO: What behavior do we expect when the scene tab is resized ?
            // Current: resize the displayed scene render image. This can cause wrong scaling, and we do not want that
            auto dim = ImGui::GetContentRegionAvail();
            auto tex = m_sceneRenderer.GetActiveImage();
            ImGui::Image((ImTextureID) tex->GetDescriptorSet(), dim);
        }
        ImGui::End();

        if (ImGui::Begin("LogsViewport", nullptr, ImGuiWindowFlags_NoTitleBar))
        {
            if (ImGui::BeginTabBar("LogsTabBar"))
            {
                if (ImGui::BeginTabItem("Logs"))
                {
                    ImGui::Text("Sample Logs");
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        if (m_pSelectedEntity != nullptr)
        {
            if (ImGui::Begin("Inspector", nullptr))
            {
                if (m_pSelectedEntity->IsSpatialEntity())
                {
                    auto transform = m_pSelectedEntity->GetRootSpatialComponent()->ModifyTransform();
                    ImGui::PushItemWidth(60);

                    // TODO: vec3 might deserve a helper function to create ui for the 3 components...
                    // Position
                    ImGui::Text("Position");
                    ImGui::DragFloat("x##Position", &transform->position.x, 1.0f);
                    ImGui::SameLine();
                    ImGui::DragFloat("y##Position", &transform->position.y, 1.0f);
                    ImGui::SameLine();
                    ImGui::DragFloat("z##Position", &transform->position.z, 1.0f);

                    // Rotation
                    ImGui::Text("Rotation");
                    ImGui::DragFloat("x##Rotation", &transform->rotation.x, 1.0f);
                    ImGui::SameLine();
                    ImGui::DragFloat("y##Rotation", &transform->rotation.y, 1.0f);
                    ImGui::SameLine();
                    ImGui::DragFloat("z##Rotation", &transform->rotation.z, 1.0f);

                    // Scale
                    ImGui::Text("Scale");
                    ImGui::DragFloat("x##Scale", &transform->scale.x, 1.0f);
                    ImGui::SameLine();
                    ImGui::DragFloat("y##Scale", &transform->scale.y, 1.0f);
                    ImGui::SameLine();
                    ImGui::DragFloat("z##Scale", &transform->scale.z, 1.0f);

                    for (auto pComponent : m_pSelectedEntity->GetComponents())
                    {
                        auto typeDesc = pComponent->GetStaticType();
                        typeDesc->InEditor(pComponent);
                    }
                }
            }
            ImGui::End();
        }

        if (ImGui::Begin("Outline"))
        {
            static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

            // TODO: How do we grab entities as a hierachy ?
            // Probably a method in entityMap
            auto& collec = m_worldEntity.GetEntitiesCollection();
            for (auto& [id, entity] : collec)
            {
                // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
                ImGuiTreeNodeFlags node_flags = base_flags;

                // const bool is_selected = (selection_mask & (1 << i)) != 0;
                if (m_pSelectedEntity != nullptr && *m_pSelectedEntity == entity)
                {
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                }

                // We add the id to the ImGui hash to differentiate entities with the same name
                std::string entityLabel = entity.GetName() + "##" + entity.GetID().ToString();
                bool node_open = ImGui::TreeNodeEx(entityLabel.c_str());

                if (ImGui::IsItemClicked())
                    m_pSelectedEntity = &entity;

                // if (test_drag_and_drop && ImGui::BeginDragDropSource())
                // {
                //     ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
                //     ImGui::Text("This is a drag and drop source");
                //     ImGui::EndDragDropSource();
                // }
                if (node_open)
                {
                    ImGui::TreePop();
                }
            }
            // m_worldEntity.
        }
        ImGui::End();

        ImGui::ShowDemoWindow();
    }
};
} // namespace aln

int main()
{
    std::unique_ptr<aln::Engine> app = std::make_unique<aln::Engine>();

    try
    {
        app->run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    app.reset();
    return EXIT_SUCCESS;
};
