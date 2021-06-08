#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <chrono> // std::chrono::seconds
#include <glm/gtx/hash.hpp>
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

#include "camera.cpp" // TODO: Create .h
#include "camera_controller.hpp"
#include "graphics/device.hpp"
#include "graphics/imgui.hpp"
#include "graphics/instance.hpp"
// #include "graphics/renderer.hpp"
#include "graphics/imgui.hpp"
#include "graphics/rendering/offline_renderer.hpp"
#include "graphics/rendering/swapchain_renderer.hpp"
#include "graphics/window.hpp"
#include "time_system.hpp"

#include "ubo.hpp"

#include "light.cpp"
#include "scene_object.cpp"
#include "skybox.cpp"
#include "utils/color_uid.cpp"
#include "vertex.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"

const std::string MODEL_PATH = "assets/models/cube.obj";
const std::string TEXTURE_PATH = "assets/textures/container2.png";
const int MAX_MODELS = 50;

class Engine
{
  public:
    Engine()
    {
        m_window.Initialize();
        m_pDevice = std::make_shared<vkg::Device>(m_window.GetSurface());
        m_swapchain = vkg::Swapchain(m_pDevice, &m_window.GetSurface(), m_window.GetWidth(), m_window.GetHeight());

        m_renderer.Create(&m_swapchain);

        m_sceneRenderer.Create(
            m_pDevice,
            m_window.GetWidth(), m_window.GetHeight(),
            2,
            m_swapchain.GetImageFormat());

        m_imgui.Initialize(m_window.GetGLFWWindow(), m_pDevice, m_renderer.GetRenderPass(), m_renderer.GetNumberOfImages());
        // TODO: Get rid of all the references to m_pDevice
        // They should not be part of this class

        loadModels();
        setUpLights();
        setupSkyBox();

        // TODO:
        //  - Do not update if no object were modified
        //  - Only update objects which have been modified
        // TODO: Let the scene handle its own descriptions (eg. do not pass each model to the swapchain like this)
    }

    void run()
    {
        mainLoop();
    }

    ~Engine()
    {
        // Cleanup vulkan objects
        clickables.clear();
        models.clear();
        selectedObject.reset();
        lightsBuffer.reset();
        lightsDescriptorSet.reset();
        skybox.reset();
    }

  private:
    vkg::Window m_window;
    std::shared_ptr<vkg::Device> m_pDevice;
    vkg::Swapchain m_swapchain;
    vkg::SwapchainRenderer m_renderer;
    vkg::OfflineRenderer m_sceneRenderer;
    vkg::ImGUI m_imgui;

    const glm::vec3 WORLD_ORIGIN = glm::vec3(0.0f);
    const glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 WORLD_BACKWARD = -WORLD_FORWARD;
    const glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 WORLD_LEFT = -WORLD_RIGHT;
    const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 WORLD_DOWN = -WORLD_UP;

    Camera camera = Camera(WORLD_BACKWARD * 2.0f);
    EditorCameraController cameraController = EditorCameraController(&camera);

    const glm::vec3 LIGHT_POSITION = glm::vec3(-4.5f);

    // Maybe unique_ptr and pass around weak_ptrs ? We pass the list to the swapchain when recording commands.
    // We also need to notify the selected field if an object is deleted
    std::vector<std::shared_ptr<SceneObject>> models;
    std::map<ColorUID, std::shared_ptr<SceneObject>> clickables; // Good candidate for weak_ptrs ?
    std::shared_ptr<SceneObject> selectedObject;
    std::vector<Light> lights;
    std::shared_ptr<Skybox> skybox;

    std::array<glm::vec3, 3> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 5.0f, -15.0f),
        LIGHT_POSITION};

    int objectToDelete = -1; // Index of objects marked for deletion
    int objectsToCreate = 0; // Number of objects to create

    // GUI toggles
    bool showTransformGUI = false;

    void addObject()
    {
        auto pos = glm::vec3(-1.5f, -2.2f, -2.5f);

        // TODO:
        std::shared_ptr<SceneObject> m = std::make_shared<SceneObject>(m_pDevice, MODEL_PATH, pos, MaterialBufferObject(), TEXTURE_PATH);
        models.push_back(m);
        clickables.insert(std::pair<ColorUID, std::shared_ptr<SceneObject>>(m->colorId, m));
    }

    void removeObject(int index)
    {
        if (models.size() > 0)
        {
            // TODO: clear selectedObject if this is this one
            clickables.erase(models[index]->colorId);
            // models[index].reset();
            models.erase(models.begin() + index);
        }
    }

    void loadModels()
    {
        for (int i = 0; i < cubePositions.size(); i++)
        {
            // TODO: This logic is a duplicate of addObject.
            auto m = std::make_shared<SceneObject>(m_pDevice, MODEL_PATH, cubePositions[i], MaterialBufferObject(), TEXTURE_PATH);
            models.push_back(m);
            clickables.insert(std::pair<ColorUID, std::shared_ptr<SceneObject>>(m->colorId, m));
        }
        selectedObject = nullptr;
    }

    void setupSkyBox()
    {
        skybox = std::make_shared<Skybox>(m_pDevice, "", MODEL_PATH);
        updateSkyboxUBO();
    }

    void updateSkyboxUBO()
    {
        vkg::UniformBufferObject ubo;
        ubo.model = glm::mat4(glm::mat3(camera.getViewMatrix()));
        ubo.view = glm::mat4(1.0f);                                                                                                            // eye/camera position, center position, up axis
        ubo.projection = glm::perspective(glm::radians(45.0f), (float) m_swapchain.GetWidth() / (float) m_swapchain.GetHeight(), 0.1f, 300.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
        ubo.projection[1][1] *= -1;                                                                                                            // GLM is designed for OpenGL which uses inverted y coordinates
        ubo.cameraPos = camera.transform.position;
        skybox->updateUniformBuffer(ubo);
    }

#pragma region lights_descriptor
    // TODO: Move lights descriptor somewhere according to requirements
    // - All the lights in a scene share a buffer (so Scene should hold them)
    // - For now swapchain holds the layout of the descriptor
    // - The descriptor itself is allocated and registered here
    vk::UniqueDescriptorSet lightsDescriptorSet;
    std::unique_ptr<vkg::Buffer> lightsBuffer;

    void createLightsBuffer()
    {
        // TODO: Handle "max lights" (rn its 5)
        lightsBuffer = std::make_unique<vkg::Buffer>(m_pDevice, 16 + (5 * sizeof(LightUniform)), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    void updateLightBufferCount(int count)
    {
        lightsBuffer->Map(0, sizeof(int)); // TODO: Respect spec alignment for int
        lightsBuffer->Copy(&count, sizeof(int));
        lightsBuffer->Unmap();
    }

    void updateLightsBuffer(Light light, int index)
    {
        auto ubo = light.getUniform();
        lightsBuffer->Map(sizeof(int) + index * sizeof(LightUniform), sizeof(LightUniform));
        lightsBuffer->Copy(&ubo, sizeof(LightUniform));
        lightsBuffer->Unmap();
    }

    void setUpLights()
    {
        // For now manually add a light
        Light light;
        light.color = glm::vec3(1.0f, 1.0f, 1.0f);
        light.position = LIGHT_POSITION;
        light.direction = WORLD_RIGHT; // Point toward 0,0,0
        light.type = LightType::Directionnal;

        lights.push_back(light);

        // Reuse the info
        // Direction is not used for point lights
        light.type = LightType::Point;
        lights.push_back(light);

        createLightsBuffer();
        createLightsDescriptorSet();

        updateLightBufferCount(lights.size());

        for (int i = 0; i < lights.size(); i++)
        {
            updateLightsBuffer(lights[i], i);
        }
    }

    // TODO: Move to light class
    void createLightsDescriptorSet()
    {
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = m_pDevice->GetDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_pDevice->GetDescriptorSetLayout<Light>();

        lightsDescriptorSet = std::move(m_pDevice->GetVkDevice().allocateDescriptorSetsUnique(allocInfo)[0]);
        m_pDevice->SetDebugUtilsObjectName(lightsDescriptorSet.get(), "Lights Descriptor Set");

        vk::DescriptorBufferInfo lightsBufferInfo;
        lightsBufferInfo.buffer = lightsBuffer->GetVkBuffer(); // TODO: How do we update the lights array ?
        lightsBufferInfo.offset = 0;
        lightsBufferInfo.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet writeDescriptor;
        writeDescriptor.dstSet = lightsDescriptorSet.get();
        writeDescriptor.dstBinding = 0;
        writeDescriptor.dstArrayElement = 0;
        writeDescriptor.descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescriptor.descriptorCount = 1;
        writeDescriptor.pBufferInfo = &lightsBufferInfo;

        m_pDevice->GetVkDevice().updateDescriptorSets(1, &writeDescriptor, 0, nullptr);
    }

#pragma endregion

    void mainLoop()
    {
        // TODO: Move to window
        while (!glfwWindowShouldClose(m_window.GetGLFWWindow()))
        {
            // std::this_thread::sleep_for(std::chrono::seconds(1));
            Time::Update();

            // Map GLFW events to the Input system
            glfwPollEvents();

            double xpos, ypos;
            glfwGetCursorPos(m_window.GetGLFWWindow(), &xpos, &ypos);
            Input::Mouse.Update({xpos, ypos});

            // Trigger input callbacks
            Input::Dispatch();

            m_renderer.BeginFrame();
            m_imgui.NewFrame();
            m_sceneRenderer.BeginFrame();

            // This is rough. TODO: Make it better:
            //  * Allow multiple objects to be deleted. Handle the object list to avoid too much overhead
            //  * Enable selecting objects to delete
            if (objectToDelete >= 0)
            {
                removeObject(objectToDelete);
                objectToDelete = -1;
            }

            // TODO: Refine as well
            //  * Change the position of the object
            //  * Specify the model
            for (int i = 0; i < objectsToCreate; i++)
            {
                addObject();
                objectsToCreate = 0;
            }

            // TODO: How do we handle lights ? It would make more sense to build a buffer once
            // The buffer should be associated in a descriptor Set.
            // Right now descriptor sets are created by model, so we would need to duplicate the light buffer -> not good !

            updateSkyboxUBO();

            for (auto model : models)
            {
                glm::mat4 modelMatrix = model->getModelMatrix();
                vkg::UniformBufferObject ubo;
                ubo.model = modelMatrix;
                // eye/camera position, center position, up axis
                ubo.view = camera.getViewMatrix();
                // 45deg vertical fov, aspect ratio, near view plane, far view plane
                ubo.projection = glm::perspective(glm::radians(45.0f), m_swapchain.GetWidth() / (float) m_swapchain.GetHeight(), 0.1f, 100.f);
                // GLM is designed for OpenGL which uses inverted y coordinates
                ubo.projection[1][1] *= -1;
                ubo.cameraPos = camera.transform.position;
                model->getComponent<Mesh>()->updateUniformBuffers(ubo);
            }

            // TODO: Handle picker again
            // if (input.isPressedLastFrame(GLFW_MOUSE_BUTTON_LEFT, true) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGui::IsAnyItemHovered())
            // {
            //     auto rgb = swapchain->picker.pickColor(models, lastMousePos);
            //     auto cID = ColorUID(rgb);
            //     // TODO: Handle background
            //     // TODO: Make sure ColorUID has a reserved id for the background
            //     selectedObject = clickables[cID];
            // }

            // swapchain->recordCommandBuffer(imageIndex, models, lightsDescriptorSet, skybox);
            m_sceneRenderer.Draw(skybox);
            m_sceneRenderer.Draw(models, lightsDescriptorSet);

            m_sceneRenderer.EndFrame();

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

        ImGui::DockSpaceOverViewport(viewport);

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

            ImGui::End();
        }

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

            ImGui::End();
        }

        if (ImGui::Begin("Scene", nullptr))
        {
            auto tex = std::dynamic_pointer_cast<vkg::Texture>(m_sceneRenderer.GetActiveImage());
            ImGui::Image((ImTextureID) tex->GetDescriptorSet(), {1600, 800});
        }
        ImGui::End();
        // if (ImGui::Begin("SceneViewport", nullptr, ImGuiWindowFlags_NoTitleBar))
        // {
        //     // TODO: Only display the viewport in the right tab.
        //     // And only if it's active
        //     auto dim = ImGui::GetWindowSize();
        //     auto pos = ImGui::GetWindowPos();

        //     if (ImGui::BeginTabBar("MainTabBar"))
        //     {
        //         if (ImGui::BeginTabItem("Scene"))
        //         {
        //             ImGui::Text("Here, the scene !");
        //             ImGui::EndTabItem();
        //         }
        //         ImGui::EndTabBar();
        //     }
        //     ImGui::End();
        // }

        // if (ImGui::Begin("LogsViewport", nullptr, ImGuiWindowFlags_NoTitleBar))
        // {
        //     // TODO: Only display the viewport in the right tab.
        //     // And only if it's active
        //     auto dim = ImGui::GetWindowSize();
        //     auto pos = ImGui::GetWindowPos();

        //     if (ImGui::BeginTabBar("LogsTabBar"))
        //     {
        //         if (ImGui::BeginTabItem("Logs"))
        //         {
        //             ImGui::Text("Sample Logs");
        //             ImGui::EndTabItem();
        //         }
        //         ImGui::EndTabBar();
        //     }
        //     ImGui::End();
        // }

        if (ImGui::Begin("Transform", nullptr) && selectedObject != nullptr)
        {
            auto transform = selectedObject->getComponent<Transform>();
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

            ImGui::End();
        }

        ImGui::End();
        ImGui::ShowDemoWindow();
    }
};

int main()
{
    std::unique_ptr<Engine> app = std::make_unique<Engine>();

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
}