// #define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

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
#include "core/buffer.hpp"
#include "core/context.hpp"
#include "core/device.hpp"
#include "core/pipeline.cpp"
#include "core/swapchain.cpp"
#include "core/texture_cubemap.hpp"
#include "ecs/common.cpp"
#include "ecs/components.hpp"
#include "ecs/coordinator.cpp"
#include "ecs/systems.cpp"
#include "input_monitor.cpp"
#include "light.cpp"
#include "scene_object.cpp"
#include "skybox.cpp"
#include "utils/color_uid.cpp"
#include "vertex.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

const int WIDTH = 800;
const int HEIGHT = 600;

const std::string MODEL_PATH = "assets/models/cube.obj";
const std::string TEXTURE_PATH = "assets/textures/container2.png";
const int MAX_MODELS = 50;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
}; // TODO: Remove when we've put this in a final location

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class Engine
{
  public:
    Engine()
    {
        initWindow();
        initVulkan();
        initImGUI();
    }

    void run()
    {
        mainLoop();
    }

    ~Engine()
    {
        // Cleanup ImGui
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Cleanup vulkan objects
        clickables.clear();
        models.clear();
        selectedObject.reset();
        lightsBuffer.reset();
        lightsDescriptorSet.reset();
        skybox.reset();
        swapchain.reset();
        context.reset();
        // TODO: The debugger is lost here. Something bad might be happening

        // Destroy the glfw context
        glfwDestroyWindow(window);
        glfwTerminate();
    }

  private:
    GLFWwindow* window;
    std::shared_ptr<core::Context> context;

    Coordinator gCoordinator;

    std::shared_ptr<core::Swapchain> swapchain;
    int frameCount = 0;

    size_t currentFrame = 0;

    bool framebufferResized = false;
    glm::vec2 lastMousePos;
    InputMonitor input;

    const glm::vec3 WORLD_ORIGIN = glm::vec3(0.0f);
    const glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 WORLD_BACKWARD = -WORLD_FORWARD;
    const glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 WORLD_LEFT = -WORLD_RIGHT;
    const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 WORLD_DOWN = -WORLD_UP;

    Camera camera = Camera(WORLD_BACKWARD * 2.0f, WORLD_UP, 90.0f, 0.0f, 0.0f);
    const glm::vec3 LIGHT_POSITION = glm::vec3(-4.5f);

    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;

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

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initWindow()
    {
        glfwInit();                                   // Init glfw
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't use OpenGL context

        // if (glfwRawMouseMotionSupported()) {
        //     glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        // }

        window = glfwCreateWindow(WIDTH, HEIGHT, "PoopyEngine", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetKeyCallback(window, keyCallback);
    }

    void initImGUI()
    {
        // Initialize Imgui context
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void) io;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo init_info;
        init_info.Instance = context->instance.get();
        init_info.PhysicalDevice = context->device->physical;
        init_info.Device = context->device->logical.get();
        init_info.QueueFamily = context->device->queueFamilyIndices.presentFamily.value();
        init_info.Queue = context->device->queues.present.queue;
        init_info.PipelineCache = nullptr;
        init_info.DescriptorPool = swapchain->descriptorPool.get();
        init_info.Allocator = nullptr;
        init_info.MinImageCount = 2;
        init_info.ImageCount = swapchain->images.size();
        init_info.CheckVkResultFn = nullptr;
        init_info.MSAASamples = (VkSampleCountFlagBits) context->device->msaaSamples;
        ImGui_ImplVulkan_Init(&init_info, swapchain->renderPass.get());

        // Upload Fonts
        // Use any command queue

        // TODO: Make the single time buffer usage more fluid
        context->device->commandpools.graphics.execute([&](vk::CommandBuffer cb) {
            ImGui_ImplVulkan_CreateFontsTexture(cb);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    // TODO: Use InputMonitor.
    // FIXME: GLFW events for keyboard and mouse might share the same identifiers
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));

        if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
        {
            app->objectsToCreate++;
        }
        else if (key == GLFW_KEY_X && action == GLFW_RELEASE)
        {
            auto index = app->models.size() - 1;
            app->objectToDelete = index;
        }
    }

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        app->camera.zoom(yoffset);
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));

        //TODO: This doesn't need to happen for every frame
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        app->lastMousePos = {xpos, ypos};
        app->input.callback(button, action, mods);
    }

    void addObject()
    {
        auto pos = glm::vec3(-1.5f, -2.2f, -2.5f);

        // TODO:
        std::shared_ptr<SceneObject> m = std::make_shared<SceneObject>(context, context->device, MODEL_PATH, pos, MaterialBufferObject(), TEXTURE_PATH);
        m->createDescriptorSet(swapchain->descriptorPool.get(), swapchain->objectsDescriptorSetLayout.get());
        m->createColorDescriptorSet(swapchain->descriptorPool.get(), swapchain->picker.descriptorSetLayout.get());
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

    // TODO
    // void initECS() {
    //     gCoordinator.init();

    //     gCoordinator.registerComponent<ecs::components::Transform>();
    //     gCoordinator.registerComponent<ecs::components::Renderable>();
    //     gCoordinator.registerComponent<Mesh>();

    //     auto renderSystem = gCoordinator.registerSystem<RenderSystem>();

    //     ecs::Signature signature;
    //     signature.set(gCoordinator.getComponentType<ecs::components::Transform>());
    //     signature.set(gCoordinator.getComponentType<ecs::components::Renderable>());
    //     signature.set(gCoordinator.getComponentType<Mesh>());
    //     gCoordinator.setSystemSignature<RenderSystem>(signature);
    // }

    void initVulkan()
    {
        context = std::make_shared<core::Context>(window);
        swapchain = std::make_shared<core::Swapchain>(std::shared_ptr(context), window, MAX_MODELS); // TODO: Swapchain are part of a Context ?

        loadModels();
        setUpLights();
        setupSkyBox();

        // Swapchain components that rely on model parameters
        // TODO:
        //  - Do not update if no object were modified
        //  - Only update objects which have been modified
        // TODO: Let the scene handle its own descriptions (eg. do not pass each model to the swapchain like this)
    }

    void loadModels()
    {
        for (int i = 0; i < cubePositions.size(); i++)
        {
            // TODO: This logic is a duplicate of addObject.
            auto m = std::make_shared<SceneObject>(context, context->device, MODEL_PATH, cubePositions[i], MaterialBufferObject(), TEXTURE_PATH);
            m->createDescriptorSet(swapchain->descriptorPool.get(), swapchain->objectsDescriptorSetLayout.get());
            m->createColorDescriptorSet(swapchain->descriptorPool.get(), swapchain->picker.descriptorSetLayout.get());
            models.push_back(m);
            clickables.insert(std::pair<ColorUID, std::shared_ptr<SceneObject>>(m->colorId, m));
        }
        selectedObject = nullptr;

        // ECS Version
        // for (int i = 0; i < N_MODELS; i++) {
        //     auto entity = gCoordinator.createEntity();
        //     gCoordinator.addComponent(entity, ecs::components::Transform {
        //         .position = cubePositions[i],
        //         .rotation = glm::vec3(0.0f),
        //         .scale = glm::vec3(1.0f)
        //     });

        //     gCoordinator.addComponent(entity, ecs::components::Renderable {
        //         .display = true,
        //         .uniform = core::Buffer(device, sizeof(core::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible)
        //     });

        //     glm::vec3 color = (i == N_MODELS-1) ? glm::vec3(1.0f, 1.0f, 1.0f): glm::vec3(1.0f, 0.5f, 0.31f); // The last object is the light
        //     // TODO: Move only mesh specific stuff to its own component
        //     gCoordinator.addComponent(entity, Mesh::fromObj(context, device, MODEL_PATH, cubePositions[i], color, Material(), TEXTURE_PATH)); // TODO: Mesh
        // }
    }

    void setupSkyBox()
    {
        skybox = std::make_shared<Skybox>(context, context->device, "", MODEL_PATH);
        skybox->createDescriptorSet(swapchain->descriptorPool.get(), swapchain->skyboxDescriptorSetLayout.get());
        updateSkyboxUBO();
    }

    void updateSkyboxUBO()
    {
        core::UniformBufferObject ubo;
        ubo.model = glm::mat4(glm::mat3(camera.getViewMatrix()));
        ubo.view = glm::mat4(1.0f);                                                                                                      // eye/camera position, center position, up axis
        ubo.projection = glm::perspective(glm::radians(45.0f), swapchain->extent.width / (float) swapchain->extent.height, 0.1f, 300.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
        ubo.projection[1][1] *= -1;                                                                                                      // GLM is designed for OpenGL which uses inverted y coordinates
        ubo.cameraPos = camera.position;
        skybox->updateUniformBuffer(ubo);
    }

#pragma region lights_descriptor
    // TODO: Move lights descriptor somewhere according to requirements
    // - All the lights in a scene share a buffer (so Scene should hold them)
    // - For now swapchain holds the layout of the descriptor
    // - The descriptor itself is allocated and registered here
    vk::UniqueDescriptorSet lightsDescriptorSet;
    std::unique_ptr<core::Buffer> lightsBuffer;

    void createLightsBuffer()
    {
        // TODO: Handle "max lights" (rn its 5)
        lightsBuffer = std::make_unique<core::Buffer>(context->device, 16 + (5 * sizeof(LightUniform)), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    void updateLightBufferCount(int count)
    {
        lightsBuffer->map(0, sizeof(int)); // TODO: Respect spec alignment for int
        lightsBuffer->copy(&count, sizeof(int));
        lightsBuffer->unmap();
    }

    void updateLightsBuffer(Light light, int index)
    {
        auto ubo = light.getUniform();
        lightsBuffer->map(sizeof(int) + index * sizeof(LightUniform), sizeof(LightUniform));
        lightsBuffer->copy(&ubo, sizeof(LightUniform));
        lightsBuffer->unmap();
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

    void createLightsDescriptorSet()
    {
        vk::DescriptorSetAllocateInfo allocInfo{swapchain->descriptorPool.get(), 1, &swapchain->lightsDescriptorSetLayout.get()};
        auto sets = context->device->logical.get().allocateDescriptorSetsUnique(allocInfo);
        lightsDescriptorSet = std::move(sets[0]);

        vk::DescriptorBufferInfo lightsBufferInfo;
        lightsBufferInfo.buffer = lightsBuffer->buffer.get(); // TODO: How do we update the lights array ?
        lightsBufferInfo.offset = 0;
        lightsBufferInfo.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet writeDescriptor;
        writeDescriptor.dstSet = lightsDescriptorSet.get();
        writeDescriptor.dstBinding = 0;
        writeDescriptor.dstArrayElement = 0;
        writeDescriptor.descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescriptor.descriptorCount = 1;
        writeDescriptor.pBufferInfo = &lightsBufferInfo;

        context->device->logical.get().updateDescriptorSets(1, &writeDescriptor, 0, nullptr);
    }

#pragma endregion

    void recreateSwapchain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        while (width == 0 || height == 0)
        { // While the window is minimized,
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents(); // Pause the app.
        }

        context->device->logical.get().waitIdle();

        // TODO: This could go inside a single function call
        swapchain->cleanup();
        swapchain->recreate(window, MAX_MODELS);
        swapchain->recordCommandBuffers(models, lightsDescriptorSet.get(), skybox);
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            // std::this_thread::sleep_for(std::chrono::seconds(1));
            frameCount++;
            // std::cout << "Frame: " << frameCount << std::endl;

            glfwPollEvents();
            if (input.isDown(GLFW_MOUSE_BUTTON_MIDDLE))
            {
                double xoffset, yoffset;
                getMouseMotionDelta(&xoffset, &yoffset);
                camera.move(-xoffset, -yoffset);
            }
            if (input.isDown(GLFW_MOUSE_BUTTON_RIGHT))
            {
                double xoffset, yoffset;
                getMouseMotionDelta(&xoffset, &yoffset);
                camera.rotate(xoffset, yoffset);
            }

            float currentFrameTime = glfwGetTime();
            deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;

            auto imageIndex = beginDrawFrame();

            processKeyboardInput(window);

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
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = model->getModelMatrix();

                core::UniformBufferObject ubo;
                ubo.model = modelMatrix;
                ubo.view = camera.getViewMatrix();                                                                                               // eye/camera position, center position, up axis
                ubo.projection = glm::perspective(glm::radians(45.0f), swapchain->extent.width / (float) swapchain->extent.height, 0.1f, 100.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
                ubo.projection[1][1] *= -1;                                                                                                      // GLM is designed for OpenGL which uses inverted y coordinates
                ubo.cameraPos = camera.position;
                model->getComponent<Mesh>()->updateUniformBuffers(ubo);
            }

            if (input.isPressedLastFrame(GLFW_MOUSE_BUTTON_LEFT, true) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && !ImGui::IsAnyItemHovered())
            {
                auto rgb = swapchain->picker.pickColor(models, lastMousePos);
                auto cID = ColorUID(rgb);
                // TODO: Handle background
                // TODO: Make sure ColorUID has a reserved id for the background
                selectedObject = clickables[cID];
            }

            swapchain->recordCommandBuffer(imageIndex, models, lightsDescriptorSet.get(), skybox);

            // Draw ImGUI components
            if (ImGui::Begin("Transform", nullptr, ImGuiWindowFlags_MenuBar) && selectedObject != nullptr)
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
            }
            ImGui::End();
            ImGui::ShowDemoWindow();
            endDrawFrame(imageIndex);
        }

        context->device->logical.get().waitIdle();
    }

    void processKeyboardInput(GLFWwindow* window)
    {
        const float cameraSpeed = 2.5f * deltaTime; // adjust accordingly
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.zoom(cameraSpeed);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.zoom(-cameraSpeed);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.move(-cameraSpeed, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.move(cameraSpeed, 0.0f);
    }

    void getMouseMotionDelta(double* dX, double* dY)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        *dX = xpos - lastMousePos.x;
        *dY = ypos - lastMousePos.y;
        lastMousePos = {xpos, ypos};
    }

    // TODO: Move to swapchain
    uint8_t beginDrawFrame()
    {
        // Wait for the fence
        context->device->logical.get().waitForFences(swapchain->inFlightFences[currentFrame].get(), VK_TRUE, UINT64_MAX);

        // Acquire an image from the swap chain
        uint32_t imageIndex;
        vk::Result result = context->device->logical.get().acquireNextImageKHR(swapchain->swapchain.get(), UINT64_MAX, swapchain->imageAvailableSemaphores[currentFrame].get(), nullptr, &imageIndex);
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapchain();
        }
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image.");
        }

        // Check if a previous frame is using the image
        if (swapchain->images[imageIndex].fence)
        {
            context->device->logical.get().waitForFences(swapchain->images[imageIndex].fence, VK_TRUE, UINT64_MAX);
        }

        // Mark the image as now being in use by this frame
        swapchain->images[imageIndex].fence = swapchain->inFlightFences[currentFrame].get();

        swapchain->beginDrawFrame(imageIndex);

        // ImGui
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        return imageIndex;
    }

    void endDrawFrame(uint32_t imageIndex)
    {
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, swapchain->images[imageIndex].commandbuffer.get());

        swapchain->endDrawFrame(imageIndex);
        vk::SubmitInfo submitInfo;

        // At which stage should we wait for each semaphores (in the same order)
        vk::Semaphore waitSemaphores = {swapchain->imageAvailableSemaphores[currentFrame].get()};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphores; // Which semaphores to wait for
        submitInfo.pWaitDstStageMask = waitStages;    // In which stage of the pipeline to wait
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &swapchain->images[imageIndex].commandbuffer.get();

        // Which semaphores to signal when job is done
        vk::Semaphore signalSemaphores[] = {swapchain->renderFinishedSemaphores[currentFrame].get()};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &swapchain->renderFinishedSemaphores[currentFrame].get();

        context->device->logical.get().resetFences(swapchain->inFlightFences[currentFrame].get());
        context->device->queues.graphics.queue.submit(submitInfo, swapchain->inFlightFences[currentFrame].get());

        vk::PresentInfoKHR presentInfo;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain->swapchain.get();
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &swapchain->renderFinishedSemaphores[currentFrame].get();
        presentInfo.pResults = nullptr; // For checking every individual swap chain results. We only have one so we don't need it

        bool recreationNeeded = false;
        vk::Result result;
        try
        {
            result = context->device->queues.graphics.queue.presentKHR(presentInfo);
        }
        catch (vk::OutOfDateKHRError const& e)
        {
            result = vk::Result::eErrorOutOfDateKHR;
        }

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
        {
            framebufferResized = true;
            recreateSwapchain();
        }
        else if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to present swap chain image.");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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