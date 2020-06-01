// #define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <fstream>
#include <array>
#include <unordered_map>
#include <string.h>

#include "vertex.hpp"
#include "camera.cpp" // TODO: Create .h
#include "mesh.cpp" // TODO: create.h
#include "core/device.hpp"
#include "core/context.hpp"
#include "core/swapchain.cpp"
#include "core/buffer.cpp"
#include "core/pipeline.cpp"
#include "light.cpp"
#include "core/textureCubeMap.cpp"
#include "ecs/coordinator.cpp"
#include "ecs/components.hpp"
#include "ecs/common.cpp"
#include "ecs/systems.cpp"
#include "skybox.cpp"

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

class Engine {
public:
    Engine() {
        initWindow();
        initVulkan();
    }
    void run() {
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    std::shared_ptr<core::Context> context;
    
    std::shared_ptr<core::Device> device;

    Coordinator gCoordinator;

    core::Swapchain swapchain;

    size_t currentFrame = 0;
    
    bool framebufferResized = false;
    bool leftMouseButtonPressed = false;
    bool rightMouseButtonPressed = false;
    bool middleMouseButtonPressed = false;
    glm::vec2 lastMousePos;

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

    std::vector<Mesh> models;
    std::vector<Light> lights; 
    Skybox skybox;

    std::array<glm::vec3, 4> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3( 2.0f, 5.0f, -15.0f),
        // ,
        LIGHT_POSITION
    };

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    } 

    void initWindow() {
        glfwInit(); // Init glfw
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't use OpenGL context

        // if (glfwRawMouseMotionSupported()) {
        //     glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        // } 

        window = glfwCreateWindow(WIDTH, HEIGHT, "PoopyEngine", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
        auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        app->camera.zoom(yoffset);
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        
        //TODO: This doesn't need to happen when pressing every buttons, only the ones that handle camera motion
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        app->lastMousePos = {xpos, ypos};

        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                if (action == GLFW_PRESS) {
                    app->leftMouseButtonPressed = true;
                } else if (action == GLFW_RELEASE) {
                    app->leftMouseButtonPressed = false;
                }
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                if (action == GLFW_PRESS) {
                    app->rightMouseButtonPressed = true;
                } else if (action == GLFW_RELEASE) {
                    app->rightMouseButtonPressed = false;
                }
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                if (action == GLFW_PRESS) {
                    app->middleMouseButtonPressed = true;
                } else if (action == GLFW_RELEASE) {
                    app->middleMouseButtonPressed = false;
                }
        }
    }

    void initECS() {
        gCoordinator.init();

        gCoordinator.registerComponent<ecs::components::Transform>();
        gCoordinator.registerComponent<ecs::components::Renderable>();
        gCoordinator.registerComponent<Mesh>();

        auto renderSystem = gCoordinator.registerSystem<RenderSystem>();
       
        ecs::Signature signature;
        signature.set(gCoordinator.getComponentType<ecs::components::Transform>());
        signature.set(gCoordinator.getComponentType<ecs::components::Renderable>());
        signature.set(gCoordinator.getComponentType<Mesh>());
        gCoordinator.setSystemSignature<RenderSystem>(signature);
    }

    void initVulkan() {
        context = std::make_shared<core::Context>();
        swapchain.createSurface(context, window); // TODO: This is dirty : device needs an initialized surface to check for extensions support,
        // but surface is contained in swapchain which require device to be initialized.
        
        device = std::make_shared<core::Device>(context->instance.get(), swapchain.surface);
        context->createCommandPools(device);

        swapchain.init(device, context->graphicsCommandPool, window, MAX_MODELS); // TODO: Swapchain are part of a Context ?
        
        /* Application related stuff */
        loadModels();
        setUpLights();
        setupSkyBox();

        /* Swapchain components that rely on model parameters */
        // TODO: 1 - At each frame, record the command buffers to update new/deleted objects
        //       2 - Do not update if no object were modified
        //       3 - Only update objects which have been modified
        // TODO: Let the scene handle its own descriptions (eg. do not pass each model to the swapchain like this)
        // TODO: Skybox is a sceneobject with a mesh and a cubemap texture, BUT it should be unique
        swapchain.recordCommandBuffers(models, lightsDescriptorSet, skybox);
    }

    void loadModels() {
        for (int i = 0; i < cubePositions.size(); i++) {
            // TODO : Clean this up.
            glm::vec3 color = (i ==  cubePositions.size() -1) ? glm::vec3(1.0f, 1.0f, 1.0f): glm::vec3(1.0f, 0.5f, 0.31f); // The last object is the light 
            auto m = Mesh::fromObj(context, device, MODEL_PATH, cubePositions[i], color, MaterialBufferObject(), TEXTURE_PATH);
            m.createDescriptorSet(swapchain.descriptorPool, swapchain.objectsDescriptorSetLayout);
            models.push_back(m);
        }

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

    void setupSkyBox() {
        skybox = Skybox(context, device, "", MODEL_PATH);
        skybox.createDescriptorSet(swapchain.descriptorPool, swapchain.skyboxDescriptorSetLayout);
        updateSkyboxUBO();
    }

    void updateSkyboxUBO() {
        core::UniformBufferObject ubo;
        ubo.model = glm::mat4(glm::mat3(camera.getViewMatrix()));
        ubo.view = glm::mat4(1.0f); // eye/camera position, center position, up axis
        ubo.projection = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float) swapchain.extent.height, 0.1f, 300.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
        ubo.projection[1][1] *= -1; // GLM is designed for OpenGL which uses inverted y coordinates
        ubo.cameraPos = camera.position;
        skybox.updateUniformBuffer(ubo);
    }

#pragma region  lights_descriptor
// TODO: Move lights descriptor somewhere according to requirements
// - All the lights in a scene share a buffer (so Scene should hold them) 
// - For now swapchain holds the layout of the descriptor
// - The descriptor itself is allocated and registered here
//  
//
    vk::DescriptorSet lightsDescriptorSet;
    core::Buffer lightsBuffer;

    void createLightsBuffer() {
        // TODO: Handle "max lights" (rn its 5)
        lightsBuffer = core::Buffer(device, 16 + (5 * sizeof(LightUniform)), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    void updateLightBufferCount(int count) {
        lightsBuffer.map(0, sizeof(int)); // TODO: Respect spec alignment for int
        lightsBuffer.copy(&count, sizeof(int));
        lightsBuffer.unmap();
    }

    void updateLightsBuffer(Light light, int index) {
        auto ubo = light.getUniform();
        lightsBuffer.map(sizeof(int) + index * sizeof(LightUniform), sizeof(LightUniform));
        lightsBuffer.copy(&ubo, sizeof(LightUniform));
        lightsBuffer.unmap();
    }

    void setUpLights() {
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

        for (int i = 0; i < lights.size(); i++) {
            updateLightsBuffer(lights[i], i);
        }

    }

    void createLightsDescriptorSet() {
        vk::DescriptorSetAllocateInfo allocInfo{ swapchain.descriptorPool, 1, &swapchain.lightsDescriptorSetLayout };
        lightsDescriptorSet = device->logicalDevice.allocateDescriptorSets(allocInfo)[0];

        vk::DescriptorBufferInfo lightsBufferInfo;
        lightsBufferInfo.buffer = lightsBuffer.buffer; // TODO: How do we update the lights array ?
        lightsBufferInfo.offset = 0;
        lightsBufferInfo.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet writeDescriptor;
        writeDescriptor.dstSet = lightsDescriptorSet;
        writeDescriptor.dstBinding = 0;
        writeDescriptor.dstArrayElement = 0;
        writeDescriptor.descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescriptor.descriptorCount = 1;
        writeDescriptor.pBufferInfo = &lightsBufferInfo;

        device->logicalDevice.updateDescriptorSets(1, &writeDescriptor, 0, nullptr);
    }

#pragma endregion

    void recreateSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        
        while (width == 0 || height == 0) { // While the window is minimized,
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents(); // Pause the app.
        }

        device->logicalDevice.waitIdle();

        swapchain.cleanup();
        
        swapchain.recreate(window, context->graphicsCommandPool, MAX_MODELS);
        swapchain.recordCommandBuffers(models, lightsDescriptorSet, skybox);
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            if (middleMouseButtonPressed) {
                double xoffset, yoffset;
                getMouseMotionDelta(&xoffset, &yoffset);
                camera.move(-xoffset, -yoffset);
            }
            if (rightMouseButtonPressed) {
                double xoffset, yoffset;
                getMouseMotionDelta(&xoffset, &yoffset);
                camera.rotate(xoffset, yoffset);
            }
            float currentFrameTime = glfwGetTime();
            deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;

            auto imageIndex = beginDrawFrame();
            
            processKeyboardInput(window);

            // TODO: How do we handle lights ? It would make more sense to build a buffer once
            // The buffer should be associated in a descriptor Set. 
            // Right now descriptor sets are created by model, so we would need to duplicate the light buffer -> not good !

            updateSkyboxUBO();

            for (auto model : models) {
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = model.getModelMatrix();
                
                core::UniformBufferObject ubo;
                ubo.model = modelMatrix;
                ubo.view = camera.getViewMatrix(); // eye/camera position, center position, up axis
                ubo.projection = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float) swapchain.extent.height, 0.1f, 100.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
                ubo.projection[1][1] *= -1; // GLM is designed for OpenGL which uses inverted y coordinates
                ubo.cameraPos = camera.position;
                model.updateUniformBuffers(ubo);

                //Material doesn't change
                // Material material;
                // material.ambient = glm::vec3(1.0f, 0.5f, 0.31f);
                // material.diffuse = glm::vec3(1.0f, 0.5f, 0.31);
                // material.specular = glm::vec3(0.5f, 0.5f, 0.5);
                // material.shininess = glm::vec1(8.0f);
                // models[i].updateMaterialBuffer(material);
            }
            swapchain.recordCommandBuffer(imageIndex, models, lightsDescriptorSet, skybox);
            endDrawFrame(imageIndex);
        }

        device->logicalDevice.waitIdle();
    }

    void processKeyboardInput(GLFWwindow *window) {
        const float cameraSpeed = 2.5f * deltaTime; // adjust accordingly
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.zoom(cameraSpeed);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.zoom(-cameraSpeed);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.move(-cameraSpeed, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.move(cameraSpeed, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            addObject();
    }

    void getMouseMotionDelta(double *dX, double *dY) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        *dX = xpos - lastMousePos.x;
        *dY = ypos - lastMousePos.y;
        lastMousePos = {xpos, ypos}; 
    }

    void addObject() {
        // std::cout << "Add object" << std::endl;
        auto pos = glm::vec3(-1.5f, -2.2f, -2.5f);
        glm::vec3 color = glm::vec3(1.0f, 0.5f, 0.31f);

        auto m = Mesh::fromObj(context, device, MODEL_PATH, pos, color, MaterialBufferObject(), TEXTURE_PATH);
        m.createDescriptorSet(swapchain.descriptorPool, swapchain.objectsDescriptorSetLayout);
        models.push_back(m);
    }

    uint8_t beginDrawFrame() {
        // Wait for the fence
        device->logicalDevice.waitForFences(swapchain.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        // Acquire an image from the swap chain
        uint32_t imageIndex;
        vk::Result result = device->logicalDevice.acquireNextImageKHR(swapchain.swapchain, UINT64_MAX, swapchain.imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);
        
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapchain();
        } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire swap chain image.");
        }

        // Check if a previous frame is using the image
        if (swapchain.images[imageIndex].fence) {
            device->logicalDevice.waitForFences(swapchain.images[imageIndex].fence, VK_TRUE, UINT64_MAX);
        }

        // Mark the image as now being in use by this frame
        swapchain.images[imageIndex].fence = swapchain.inFlightFences[currentFrame];
        return imageIndex;
    }

    void endDrawFrame(uint32_t imageIndex) {
        vk::SubmitInfo submitInfo;
        
        // At which stage should we wait for each semaphores (in the same order)
        vk::Semaphore waitSemaphores = {swapchain.imageAvailableSemaphores[currentFrame]};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphores; // Which semaphores to wait for
        submitInfo.pWaitDstStageMask = waitStages; // In which stage of the pipeline to wait 
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &swapchain.images[imageIndex].commandbuffer;
        
        // Which semaphores to signal when job is done
        vk::Semaphore signalSemaphores[] = {swapchain.renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &swapchain.renderFinishedSemaphores[currentFrame];

        device->logicalDevice.resetFences(swapchain.inFlightFences[currentFrame]);
        device->graphicsQueue.submit(submitInfo, swapchain.inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfo;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex; 
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &swapchain.renderFinishedSemaphores[currentFrame];
        presentInfo.pResults = nullptr; // For checking every individual swap chain results. We only have one so we don't need it

        bool recreationNeeded = false;
        vk::Result result;
        try {
            result = device->graphicsQueue.presentKHR(presentInfo);
        } catch (vk::OutOfDateKHRError const &e) {
            result = vk::Result::eErrorOutOfDateKHR;
        }

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) { 
            framebufferResized = true;
            recreateSwapchain();
        } else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to present swap chain image.");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanup() {
        swapchain.destroy();

        for (auto model : models) {
            model.destroy();
        }

        skybox.destroy();
        
        device->logicalDevice.destroyCommandPool(context->graphicsCommandPool);
        device->logicalDevice.destroyCommandPool(context->transferCommandPool);
        device->logicalDevice.destroy();

        context->instance->destroySurfaceKHR(swapchain.surface);
        // TODO: either destroy surface at the same time as the rest of the swap chain,
        // or move it out.
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    Engine app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
    // TODO: There is an error when the app variable is released (i think). Why ?
    // It seems to stumble onto a segfault.
    // ideas: it might be due to one of the attributes to the Engine class not being destroyed properly. (resource already freed)
}