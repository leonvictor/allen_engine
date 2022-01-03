#include <glm/gtc/random.hpp>
#include <glm/vec3.hpp>

#include <chrono> // std::chrono::seconds
#include <thread> // std::this_thread::sleep_for

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fmt/core.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string.h>
#include <unordered_map>
#include <vector>

#include <graphics/device.hpp>
#include <graphics/imgui.hpp>
#include <graphics/instance.hpp>
#include <graphics/rendering/offline_renderer.hpp>
#include <graphics/rendering/swapchain_renderer.hpp>
#include <graphics/ubo.hpp>
#include <graphics/window.hpp>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>

#include <crtdbg.h>

#include <input/input_system.hpp>

#include <entities/entity.hpp>
#include <entities/world_entity.hpp>
#include <entities/world_update.hpp>

#include <core/camera.hpp>
#include <core/camera_controller.hpp>
#include <core/component_factory.hpp>
#include <core/light.hpp>
#include <core/mesh_renderer.hpp>
#include <core/render_system.hpp>
#include <core/systems/script.hpp>
#include <core/time_system.hpp>

#include <assets/manager.hpp>
#include <core/asset_loaders/material_loader.hpp>
#include <core/asset_loaders/mesh_loader.hpp>
#include <core/asset_loaders/texture_loader.hpp>

#include "IconsFontAwesome4.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include <config/path.h>
#include <reflection/reflection.hpp>

#include <Tracy.hpp>

using namespace aln::input;
using namespace aln::entities;

// TODO: Only when tracing memory
void* operator new(std::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
void operator delete(void* ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}

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

        // TODO: Move somewhere else
        // TODO: What's the scope ? How do we expose the asset manager ?
        auto pAssetManager = std::make_shared<AssetManager>();
        // TODO: Get rid of the default paths
        pAssetManager->RegisterAssetLoader<Mesh, MeshLoader>(m_pDevice, MODEL_PATH);
        pAssetManager->RegisterAssetLoader<Texture, TextureLoader>(m_pDevice, TEXTURE_PATH);
        pAssetManager->RegisterAssetLoader<Material, MaterialLoader>(m_pDevice);

        // Create a default context
        m_componentFactory.context = {
            .graphicsDevice = m_pDevice,
            .defaultTexturePath = TEXTURE_PATH,
            .defaultModelPath = MODEL_PATH,
            .pAssetManager = pAssetManager};

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

    std::array<glm::vec3, 2> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 5.0f, 0.0f),
        // LIGHT_POSITION
    };

    Entity* m_pSelectedEntity = nullptr;

    float m_scenePreviewWidth = 1.0f;
    float m_scenePreviewHeight = 1.0f;

    // Object model
    ComponentFactory m_componentFactory;
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
            // TODO: Refine how the editor accesses the map
            Entity* pCameraEntity = m_worldEntity.m_entityMap.CreateEntity("MainCamera");
            auto pCameraComponent = m_componentFactory.Create<Camera>();
            pCameraComponent->SetLocalTransformPosition(glm::vec3(-20.0f, 0.0f, 0.0f));
            pCameraComponent->SetLocalTransformRotationEuler(glm::vec3(90.0f, 0.0f, 0.0f));
            pCameraEntity->AddComponent(pCameraComponent);

            pCameraEntity->CreateSystem<EditorCameraController>();
        }

        {
            Entity* pLightEntity = m_worldEntity.m_entityMap.CreateEntity("DirectionalLight");
            Light* pLightComponent = m_componentFactory.Create<Light>();
            pLightComponent->direction = WORLD_RIGHT;
            pLightComponent->type = Light::Type::Directional;
            pLightComponent->SetLocalTransformPosition(LIGHT_POSITION);
            pLightEntity->AddComponent(pLightComponent);

            Entity* pPointLightEntity = m_worldEntity.m_entityMap.CreateEntity("PointLight");
            Light* pPLightComponent = m_componentFactory.Create<Light>();
            pPLightComponent->direction = WORLD_RIGHT;
            pPLightComponent->type = Light::Type::Point;
            pPLightComponent->SetLocalTransformPosition(LIGHT_POSITION);
            pPointLightEntity->AddComponent(pPLightComponent);
        }

        int i = 1;
        for (auto pos : cubePositions)
        {
            // TODO: This api is too verbose
            Entity* pCube = m_worldEntity.m_entityMap.CreateEntity(fmt::format("cube ({})", i));
            auto pMesh = m_componentFactory.Create<MeshRenderer>();
            pMesh->SetLocalTransformPosition(pos);
            pCube->AddComponent(pMesh);
            i++;
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

            m_renderer.BeginFrame(aln::vkg::render::RenderContext());
            m_imgui.NewFrame();

            // Object model: Update systems at various points in the frame.
            // TODO: Handle sync points here ?
            for (uint8_t stage = UpdateStage::FrameStart; stage != UpdateStage::NumStages; stage++)
            {
                aln::entities::UpdateContext context = aln::entities::UpdateContext(static_cast<UpdateStage>(stage));
                // When out of editor
                // context.displayWidth = m_window.GetWidth();
                // context.displayHeight = m_window.GetHeight();

                context.displayWidth = m_scenePreviewWidth;
                context.displayHeight = m_scenePreviewHeight;

                m_worldEntity.Update(context);
            }

            // updateSkyboxUBO();

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

        FrameMark;
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
                if (ImGui::BeginMenu("Debug"))
                {
                    if (ImGui::MenuItem("Lots of boxes"))
                    {
                        for (int i = 4; i < 64; i++)
                        {
                            Entity* pCube = m_worldEntity.m_entityMap.CreateEntity(fmt::format("cube ({})", i));
                            auto pos = glm::vec3(
                                glm::linearRand(-100.0f, 100.0f),
                                glm::linearRand(-100.0f, 100.0f),
                                glm::linearRand(-100.0f, 100.0f));
                            auto pMesh = m_componentFactory.Create<MeshRenderer>();
                            pMesh->SetLocalTransformPosition(pos);
                            pCube->AddComponent(pMesh);
                            pCube->CreateSystem<ScriptSystem>();
                        }
                    }
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
                ImGui::Text(fmt::format("{:.0f} FPS", 1.0f / Time::GetDeltaTime()).c_str());
                ImGui::EndMenuBar();
            }
        }
        ImGui::End();

        if (ImGui::Begin(ICON_FA_GLOBE " Scene", nullptr, ImGuiWindowFlags_NoScrollbar))
        {
            auto dim = ImGui::GetContentRegionAvail();
            m_scenePreviewWidth = dim.x;
            m_scenePreviewHeight = dim.y;
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

        // Inspector panel
        if (m_pSelectedEntity != nullptr)
        {
            if (ImGui::Begin(ICON_FA_INFO_CIRCLE " Inspector", nullptr))
            {
                ImGui::PushID(m_pSelectedEntity->GetID().ToString().c_str());

                ImGui::AlignTextToFramePadding();
                ImGui::Text(ICON_FA_CUBES);
                ImGui::SameLine();
                ImGui::InputText("", &m_pSelectedEntity->GetName());
                ImGui::SameLine(ImGui::GetWindowWidth() - 30);

                if (m_pSelectedEntity->IsActivated())
                {
                    ImGui::Text(ICON_FA_EYE);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Disable entity");
                    }
                    if (ImGui::IsItemClicked())
                    {
                        m_worldEntity.DeactivateEntity(m_pSelectedEntity);
                    }
                }
                else
                {
                    ImGui::Text(ICON_FA_EYE_SLASH);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Enable entity");
                    }
                    if (ImGui::IsItemClicked())
                    {
                        m_worldEntity.ActivateEntity(m_pSelectedEntity);
                    }
                }

                if (m_pSelectedEntity->IsSpatialEntity())
                {
                    if (ImGui::CollapsingHeader("Transform"))
                    {
                        ImGui::PushItemWidth(60);

                        // TODO: vec3 might deserve a helper function to create ui for the 3 components...
                        // Position
                        bool changed = false;
                        auto pos = m_pSelectedEntity->GetRootSpatialComponent()->GetLocalTransform().GetTranslation();
                        ImGui::Text("Position");
                        changed |= ImGui::DragFloat("x##Position", &pos.x, 1.0f);
                        ImGui::SameLine();
                        changed |= ImGui::DragFloat("y##Position", &pos.y, 1.0f);
                        ImGui::SameLine();
                        changed |= ImGui::DragFloat("z##Position", &pos.z, 1.0f);

                        if (changed)
                            m_pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformPosition(pos);

                        // Rotation
                        auto euler = m_pSelectedEntity->GetRootSpatialComponent()->GetLocalTransform().GetRotationEuler();
                        changed = false;
                        ImGui::Text("Rotation");
                        changed |= ImGui::DragFloat("x##Rotation", &euler.x, 1.0f);
                        ImGui::SameLine();
                        changed |= ImGui::DragFloat("y##Rotation", &euler.y, 1.0f);
                        ImGui::SameLine();
                        changed |= ImGui::DragFloat("z##Rotation", &euler.z, 1.0f);

                        if (changed)
                            m_pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformRotationEuler(euler);

                        // Scale
                        auto scale = m_pSelectedEntity->GetRootSpatialComponent()->GetLocalTransform().GetScale();
                        changed = false;
                        ImGui::Text("Scale");
                        changed |= ImGui::DragFloat("x##Scale", &scale.x, 1.0f);
                        ImGui::SameLine();
                        changed |= ImGui::DragFloat("y##Scale", &scale.y, 1.0f);
                        ImGui::SameLine();
                        changed |= ImGui::DragFloat("z##Scale", &scale.z, 1.0f);

                        if (changed)
                            m_pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformScale(scale);
                    }
                }

                for (auto pComponent : m_pSelectedEntity->GetComponents())
                {
                    ImGui::PushID(pComponent->GetID().ToString().c_str());

                    auto typeDesc = pComponent->GetType();
                    // TODO: This should happen through the partial template specialization for components

                    if (ImGui::CollapsingHeader(typeDesc->GetPrettyName().c_str(), ImGuiTreeNodeFlags_AllowItemOverlap))
                    {
                        typeDesc->InEditor(pComponent, typeDesc->GetPrettyName().c_str());
                    }

                    if (ImGui::BeginPopupContextItem("context_popup", ImGuiPopupFlags_MouseButtonRight))
                    {
                        if (ImGui::MenuItem("Remove Component", "", false, true))
                        {
                            m_pSelectedEntity->DestroyComponent(pComponent->GetID());
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                for (auto pSystem : m_pSelectedEntity->GetSystems())
                {
                    auto typeDesc = pSystem->GetType();

                    ImGui::PushID(typeDesc->GetPrettyName().c_str());
                    if (ImGui::CollapsingHeader(typeDesc->GetPrettyName().c_str()))
                    {
                        typeDesc->InEditor(pSystem.get());
                    }

                    if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
                    {
                        if (ImGui::MenuItem("Remove System", "", false, true))
                        {
                            m_pSelectedEntity->DestroySystem(pSystem->GetType());
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                if (ImGui::Button("Add Component"))
                {
                    // Open a dropdown with all components types
                    ImGui::OpenPopup("add_component_popup");
                }

                ImGui::SameLine();
                if (ImGui::Button("Add System"))
                {
                    ImGui::OpenPopup("add_system_popup");
                }

                if (ImGui::BeginPopup("add_system_popup"))
                {
                    auto& systemTypes = aln::reflect::GetTypesInScope("SYSTEMS");
                    for (auto& sys : systemTypes)
                    {
                        if (ImGui::Selectable(sys->GetPrettyName().c_str()))
                        {
                            m_pSelectedEntity->CreateSystem(sys);
                        }
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::BeginPopup("add_component_popup"))
                {
                    auto& componentTypes = aln::reflect::GetTypesInScope("COMPONENTS");
                    for (auto& comp : componentTypes)
                    {
                        if (ImGui::Selectable(comp->GetPrettyName().c_str()))
                        {
                            auto newComp = m_componentFactory.Create(comp);
                            m_pSelectedEntity->AddComponent(newComp);
                        }
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }
            ImGui::End();
        }

        // Outline panel
        if (ImGui::Begin(ICON_FA_LIST " Outline"))
        {
            auto& tree = m_worldEntity.GetEntityTree();
            for (Entity* node : tree)
            {
                RecurseEntityTree(node);
            }

            // Add a dummy panel to the rest of the outline pane
            ImGui::Dummy(ImGui::GetWindowSize());

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
                {
                    assert(payload->DataSize == sizeof(Entity**));
                    Entity* entityPayload = *((Entity**) payload->Data);
                    entityPayload->SetParentEntity(nullptr);
                }
                ImGui::EndDragDropTarget();
            }

            EntityOutlinePopup(nullptr);
        }
        ImGui::End();

        ImGui::ShowDemoWindow();
    }

    void EntityOutlinePopup(Entity* pEntity = nullptr)
    {
        if (ImGui::BeginPopupContextItem("entity_outline_popup", ImGuiPopupFlags_MouseButtonRight))
        {
            auto contextEntityAndNotSpatial = (pEntity != nullptr && !pEntity->IsSpatialEntity());

            if (pEntity != nullptr)
            {
                ImGui::Text(pEntity->GetID().ToString().c_str());
                if (ImGui::MenuItem("Remove Entity"))
                {
                    if (m_pSelectedEntity == pEntity)
                    {
                        m_pSelectedEntity = nullptr;
                    }
                    m_worldEntity.m_entityMap.RemoveEntity(pEntity);
                }
            }
            if (ImGui::MenuItem("Add Empty Entity", "", false, pEntity == nullptr))
            {
                auto* pNewEntity = m_worldEntity.m_entityMap.CreateEntity("Entity");
                // TODO: This will be useful with other options, but empty entities are not spatial so they can't be attached to others.
                // if (pEntity != nullptr)
                // {
                //     pNewEntity->SetParentEntity(pEntity);
                // }
            }
            ImGui::EndPopup();
        }
    }

    void RecurseEntityTree(Entity* pEntity)
    {
        ImGui::PushID(pEntity->GetID().ToString().c_str());
        static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
        ImGuiTreeNodeFlags node_flags = base_flags;

        if (m_pSelectedEntity != nullptr && m_pSelectedEntity == pEntity)
        {
            node_flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool hasChildren = pEntity->HasChildrenEntities();
        if (!hasChildren)
        {
            node_flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (!pEntity->IsActivated())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }

        // We add the id to the ImGui hash to differentiate entities with the same name
        bool node_open = ImGui::TreeNodeEx(pEntity->GetName().c_str(), node_flags);

        EntityOutlinePopup(pEntity);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            m_pSelectedEntity = pEntity;

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("ENTITY", &pEntity, sizeof(Entity**));
            ImGui::Text(pEntity->GetName().c_str());
            ImGui::EndDragDropSource();
        }

        // Entity drag and drop target
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                assert(payload->DataSize == sizeof(Entity**));

                Entity* entityPayload = *((Entity**) payload->Data);

                if (entityPayload->IsSpatialEntity())
                {
                    entityPayload->SetParentEntity(pEntity);

                    // Set the receiving node as open
                    ImGui::GetStateStorage()->SetInt(ImGui::GetID(pEntity->GetName().c_str()), 1);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (node_open && hasChildren)
        {
            ImGui::Indent();
            for (auto child : pEntity->GetChildren())
            {
                RecurseEntityTree(child);
            }
            ImGui::Unindent();
        }

        if (!pEntity->IsActivated())
        {
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
    }
};
} // namespace aln

int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

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
