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

// #define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>

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
#include "core/descriptor.cpp"
#include "core/pipeline.cpp"
#include "core/texture.cpp"

const int WIDTH = 800;
const int HEIGHT = 600;

const std::string MODEL_PATH = "assets/models/cube.obj";
const std::string TEXTURE_PATH = "assets/textures/camel.jpg";

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

    core::Swapchain swapchain;

    size_t currentFrame = 0;
    
    std::shared_ptr<core::Buffer> vertexBuffer;
    std::shared_ptr<core::Buffer> indexBuffer;

    uint32_t textureMipLevels;

    std::shared_ptr<core::Texture> texture;

    bool framebufferResized = false;
    bool leftMouseButtonPressed = false;
    bool rightMouseButtonPressed = false;
    bool middleMouseButtonPressed = false;
    glm::vec2 lastMousePos;

    const glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 WORLD_BACKWARD = -WORLD_FORWARD;
    const glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 WORLD_LEFT = -WORLD_RIGHT;
    const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 WORLD_DOWN = -WORLD_UP;

    Camera camera = Camera(WORLD_BACKWARD * 2.0f, WORLD_UP, 90.0f, 0.0f, 0.0f);

    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;

    Mesh model;

    std::array<glm::vec3, 3> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3( 2.0f, 5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f)
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

    void initVulkan() {
        context = std::make_shared<core::Context>();
        swapchain.createSurface(context, window); // TODO: This is dirty : device needs an initialized surface to check for extensions support,
        // but surface is contained in swapchain which require device to be initialized.
        
        device = std::make_shared<core::Device>(context->instance.get(), swapchain.surface);
        context->createCommandPools(device);

        swapchain.init(device, window); // TODO: Swapchain are part of a Context ?
        
        /* Model attributes */
        texture = std::make_shared<core::Texture>(context, device, TEXTURE_PATH);
        
        /* Application related stuff. How do we handle multiple a complex scene with multiple models ? */
        loadModel();
        createVertexBuffer();
        createIndexBuffer();

        /* Swapchain components that rely on model parameters */
        swapchain.createDescriptorSets(*texture);
        swapchain.createCommandBuffers(context->graphicsCommandPool, *vertexBuffer, *indexBuffer, model.indices.size());
    }

    void loadModel() {
        model = Mesh::fromObj(MODEL_PATH);
    }

    void createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(model.vertices[0]) * model.vertices.size();
        
        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(model.vertices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        vertexBuffer = std::make_shared<core::Buffer>(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);   
        context->copyBuffer(stagingBuffer, *vertexBuffer, bufferSize);

        stagingBuffer.destroy();
    }

    void createIndexBuffer() {
        vk::DeviceSize  bufferSize = sizeof(model.indices[0]) * model.indices.size();
        
        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(model.indices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        indexBuffer = std::make_shared<core::Buffer>(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);   
        context->copyBuffer(stagingBuffer, *indexBuffer, bufferSize);

        stagingBuffer.destroy();
    }

    void recreateSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        
        while (width == 0 || height == 0) { // While the window is minimized,
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents(); // Pause the app.
        }

        device->logicalDevice.waitIdle();

        swapchain.cleanup();
        
        swapchain.recreate(window);
        // TODO: Those two calls should be inside recreate() but require too many parameters for now...
        swapchain.createDescriptorSets(*texture); 
        swapchain.createCommandBuffers(context->graphicsCommandPool, *vertexBuffer, *indexBuffer, model.indices.size());
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

            for (int i = 0; i < 1; i++) { // TODO: Just one to display smth
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, cubePositions[i]);
                
                core::UniformBufferObject ubo;;
                ubo.model = modelMatrix;
                ubo.view = camera.getViewMatrix(); // eye/camera position, center position, up axis
                ubo.projection = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float) swapchain.extent.height, 0.1f, 10.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
                ubo.projection[1][1] *= -1; // GLM is designed for OpenGL which uses inverted y coordinates
                
                swapchain.updateUniformBuffers(imageIndex, ubo);
            }
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
    }

    void getMouseMotionDelta(double *dX, double *dY) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        *dX = xpos - lastMousePos.x;
        *dY = ypos - lastMousePos.y;
        lastMousePos = {xpos, ypos}; 
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

        texture->destroy();

        vertexBuffer->destroy();
        indexBuffer->destroy();

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