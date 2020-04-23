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
#include "core/renderpass.hpp"
#include "core/buffer.cpp"
#include "core/descriptor.cpp"
#include "core/pipeline.cpp"
#include "core/texture.cpp"

const int WIDTH = 800;
const int HEIGHT = 600;

const std::string MODEL_PATH = "assets/models/cube.obj";
const std::string TEXTURE_PATH = "assets/textures/camel.jpg";

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
}; // TODO: Remove when we've put this in a final location

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection; 
};

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

    core::RenderPass renderPass;
    
    core::Descriptor descriptor;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    core::Pipeline graphicsPipeline;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    size_t currentFrame = 0;
    
    std::shared_ptr<core::Buffer> vertexBuffer;
    std::shared_ptr<core::Buffer> indexBuffer;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkBuffer lightUniformBuffer;
    VkDeviceMemory lightUniformBufferMemory;

    uint32_t textureMipLevels;

    std::shared_ptr<core::Texture> texture;

    // VkSampler textureSampler;

    bool framebufferResized = false;
    bool leftMouseButtonPressed = false;
    bool rightMouseButtonPressed = false;
    bool middleMouseButtonPressed = false;
    glm::vec2 lastMousePos;

    const glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 WORLD_BACKWARD = glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 WORLD_LEFT = glm::vec3(-1.0f, 0.0f, 0.0f);
    const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 WORLD_DOWN = glm::vec3(0.0f, -1.0f, 0.0f);

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
        swapchain.init(device, window); // TODO: Swapchain are part of a Context
        renderPass.init(device, swapchain);
        descriptor = core::Descriptor(device, static_cast<uint32_t>(swapchain.images.size()));
        descriptor.createDescriptorSetLayout(); // The layout is used by the graphics pipeline

        graphicsPipeline.createGraphicsPipeline(device, swapchain.extent, descriptor.setLayout, renderPass.renderPass);
        swapchain.initFramebuffers(renderPass.renderPass);
        context->createCommandPools(device);
        texture = std::make_shared<core::Texture>(context, device, TEXTURE_PATH);
        // createTextureSampler();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void loadModel() {
        model = Mesh::fromObj(MODEL_PATH);
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);
            if (tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features) {
                return format;
            }

            throw std::runtime_error("Failed to find a supported format.");
            
        }
        VkFormat truc;
        return truc;
    }

    void createDescriptorSets() {
        // std::vector<VkDescriptorSetLayout> layouts(swapchain.images.size(), descriptorSetLayout);
        std::vector<VkDescriptorSetLayout> layouts(swapchain.images.size(), descriptor.setLayout);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchain.images.size());
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(swapchain.images.size());
        if (vkAllocateDescriptorSets(device->getCDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor set.");
        }

        for (size_t i = 0; i < swapchain.images.size(); i++) {
            // Describes the buffer and the region within it that contains the data for the descriptor
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture->image.view;
            imageInfo.sampler = texture->sampler;

            std::array<VkWriteDescriptorSet, 2> writeDescriptors = {};
            writeDescriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptors[0].dstSet = descriptorSets[i];
            writeDescriptors[0].dstBinding = 0; // Binding index 
            writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
            writeDescriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptors[0].descriptorCount = 1;
            writeDescriptors[0].pBufferInfo = &bufferInfo;

            writeDescriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptors[1].dstSet = descriptorSets[i];
            writeDescriptors[1].dstBinding = 1; // Binding index 
            writeDescriptors[1].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
            writeDescriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptors[1].descriptorCount = 1;
            writeDescriptors[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device->getCDevice(), static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchain.images.size());
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchain.images.size());

        VkDescriptorPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        createInfo.pPoolSizes = poolSizes.data();
        createInfo.maxSets = static_cast<uint32_t>(swapchain.images.size());

        if (vkCreateDescriptorPool(device->getCDevice(), &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool");
        }
    }

    /* TODO : Moved to buffer.cpp, remove when done transfering */
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        // TODO: Move queues out of this function
        vk::SurfaceKHR pSurface = vk::SurfaceKHR(swapchain.surface);
        core::QueueFamilyIndices queueFamilyIndices = core::findQueueFamilies(device->physicalDevice, pSurface);
        uint32_t queues[] = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.transferFamily.value()};
        
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size; // Byte size of the buffer = vertex size
        bufferInfo.usage = usage; // This is a vertex buffer
        // TODO: We might have to pull this out as well
        bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT; // Can buffers be shared between queues?
        bufferInfo.flags = 0; // Configure sparse buffer memory. Not used rn
        
        bufferInfo.queueFamilyIndexCount = 2;
        bufferInfo.pQueueFamilyIndices = queues;
    
        if (vkCreateBuffer(device->getCDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vertex buffer.");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device->getCDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memProperties);

        if (vkAllocateMemory(device->getCDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate memory for buffer.");
        }

        vkBindBufferMemory(device->getCDevice(), buffer, bufferMemory, 0);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        
        uniformBuffers.resize(swapchain.images.size());
        uniformBuffersMemory.resize(swapchain.images.size());

        for (size_t i = 0; i < swapchain.images.size(); i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
        }
    }

    void updateUniformBuffers(uint32_t currentImage, UniformBufferObject ubo) { 
        void* data;
        vkMapMemory(device->getCDevice(), uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device->getCDevice(), uniformBuffersMemory[currentImage]); // TODO: this is unefficient. Use push constants to pass a small buffer of data to shaders
    }   

    void createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(model.vertices[0]) * model.vertices.size();
        
        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(model.vertices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        vertexBuffer = std::make_shared<core::Buffer>(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);   
        context->copyBuffer(stagingBuffer, *vertexBuffer, bufferSize);
    }

    void createIndexBuffer() {
        vk::DeviceSize  bufferSize = sizeof(model.indices[0]) * model.indices.size();
        
        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(model.indices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        // createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
        indexBuffer = std::make_shared<core::Buffer>(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);   
        context->copyBuffer(stagingBuffer, *indexBuffer, bufferSize);

        // vkDestroyBuffer(device->getCDevice(), stagingBuffer, nullptr);
        // vkFreeMemory(device->getCDevice(), stagingBufferMemory, nullptr);
    }

    
    // Moved to device
    /* Combine memory requirements of a buffer, the app requirements and the physical device's property to find the right type of memory to use. 
     * @args:
     *      typeFilter : Suitable bit field of memory types
     *      properties : Required properties
     */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device->physicalDevice, &memProperties);
        
        // TODO sometimes, consider heaps...
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (typeFilter & (1 << i) && // Check if the memory type's bit is set to 1
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) { // We also need to be able to write our vertex data to the memory
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type.");

    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(swapchain.images.size(), VK_NULL_HANDLE); // ?
        
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Create them in the signaled state
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            if (vkCreateSemaphore(device->getCDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device->getCDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device->getCDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                
                throw std::runtime_error("Failed to create sync objects for a frame.");
            }
        }
    }

    void createCommandBuffers() {
        // commandBuffers.resize(swapchainFramebuffers.size());
        commandBuffers.resize(swapchain.images.size());

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        // allocInfo.commandPool = graphicsCommandPool;
        allocInfo.commandPool = VkCommandPool(context->graphicsCommandPool);
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Or secondary

        if (vkAllocateCommandBuffers(device->getCDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers.");
        }

        // TODO: Create command buffers for the transfer queue ?

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
            cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffers[i], &cmdBufferBeginInfo) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin recording command buffer.");
            }

            // Start a render pass. TODO: Why is that here ?
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass.renderPass;
            // renderPassInfo.framebuffer = swapchainFramebuffers[i];
            renderPassInfo.framebuffer = swapchain.images[i].framebuffer;
            renderPassInfo.renderArea.extent = swapchain.extent;
            renderPassInfo.renderArea.offset = {0, 0};

            std::array<VkClearValue, 2> clearValues = {};
            clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.graphicsPipeline);
            
            // VkBuffer vertexBuffers[] = {VkBuffer(vertexBuffer)}; // TODO
            VkBuffer vertexBuffers[] = {VkBuffer(vertexBuffer->buffer)}; // TODO
            VkDeviceSize offsets[] = {0};
            
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            
            // vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.layout, 0, 1, &descriptorSets[i], 0, nullptr);

            vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
            
            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to record command buffer.");
            }

        }
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device->getCDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("ShaderModule creation failed.");
        }
        return shaderModule;
    }

    void recreateSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) { // While the window is minimized,
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents(); // Pause the app.
        }

        vkDeviceWaitIdle(device->getCDevice());

        cleanupSwapchain();
        swapchain.init(device, window);
        renderPass.init(device, swapchain);
        // createRenderPass();
        graphicsPipeline.createGraphicsPipeline(device, swapchain.extent, descriptor.setLayout, renderPass.renderPass);
        // createFramebuffers();
        swapchain.initFramebuffers(renderPass.renderPass);

        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
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
                UniformBufferObject ubo = {};
                // ubo.model = model.getModelMatrix();
                ubo.model = modelMatrix;
                ubo.view = camera.getViewMatrix(); // eye/camera position, center position, up axis
                ubo.projection = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float) swapchain.extent.height, 0.1f, 10.f); // 45deg vertical fov, aspect ratio, near view plane, far view plane
                ubo.projection[1][1] *= -1; // GLM is designed for OpenGL which uses inverted y coordinates
                
                updateUniformBuffers(imageIndex, ubo);
            }
            endDrawFrame(imageIndex);
        }

        vkDeviceWaitIdle(device->getCDevice());
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
        vkWaitForFences(device->getCDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        // Acquire an image from the swap chain
        uint32_t imageIndex;

        VkResult result = vkAcquireNextImageKHR(device->getCDevice(), swapchain.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain image.");
        }

        // Check if a previous frame is using the image
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device->getCDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        // Mark the image as now being in use by this frame
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];
        return imageIndex;
    }

    void endDrawFrame(uint32_t imageIndex) {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        // At which stage should we wait for each semaphores (in the same order)
        VkSemaphore waitSemaphores = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphores; // Which semaphores to wait for
        submitInfo.pWaitDstStageMask = waitStages; // In which stage of the pipeline to wait 
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        
        // Which semaphores to signal when job is done
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(device->getCDevice(), 1, &inFlightFences[currentFrame]); // Revert to unsignaled state
        
        if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer");
        } 

        // Submit the result back to the swap chain
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        VkSwapchainKHR swapchains[] = {swapchain.swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex; 
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.pResults = nullptr; // For checking every individual swap chain results. We only have one so we don't need it

        VkQueue pQueue = VkQueue(device->graphicsQueue); // TODO: We're just casting for now
        VkResult result = vkQueuePresentKHR(pQueue, &presentInfo); 
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = true;
            recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image.");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanupSwapchain() {
        vkDestroyImageView(device->getCDevice(), swapchain.colorImage.view, nullptr);
        vkDestroyImage(device->getCDevice(), swapchain.colorImage.image, nullptr);
        vkFreeMemory(device->getCDevice(), swapchain.colorImage.memory, nullptr);

        // TODO: Move this out
        vkDestroyImageView(device->getCDevice(), swapchain.depthImage.view, nullptr);
        vkDestroyImage(device->getCDevice(), swapchain.depthImage.image, nullptr);
        vkFreeMemory(device->getCDevice(), swapchain.depthImage.memory, nullptr);

        // for (auto framebuffer : swapchainFramebuffers) {
        //     vkDestroyFramebuffer(device->getCDevice(), framebuffer, nullptr);
        // }

        for (auto img : swapchain.images) {
            vkDestroyFramebuffer(device->getCDevice(), img.framebuffer, nullptr);
        }
        
        // vkFreeCommandBuffers(device->getCDevice(), graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        vkFreeCommandBuffers(device->getCDevice(), VkCommandPool(context->graphicsCommandPool), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

        vkDestroyPipeline(device->getCDevice(), graphicsPipeline.graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device->getCDevice(), graphicsPipeline.layout, nullptr);

        vkDestroyRenderPass(device->getCDevice(), renderPass, nullptr);

        for (auto image : swapchain.images) {
            vkDestroyImageView(device->getCDevice(), image.imageView, nullptr);
        }

        vkDestroySwapchainKHR(device->getCDevice(), swapchain.swapchain, nullptr);

        // Should this be before swap chain destruction ?
        for (size_t i = 0; i < swapchain.images.size(); i++) {
            vkDestroyBuffer(device->getCDevice(), uniformBuffers[i], nullptr);
            vkFreeMemory(device->getCDevice(), uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device->getCDevice(), descriptorPool, nullptr);
    }

    void cleanup() {
        cleanupSwapchain();

        vkDestroySampler(device->getCDevice(), texture->sampler, nullptr);
        vkDestroyImageView(device->getCDevice(), texture->image.view, nullptr);
        vkDestroyImage(device->getCDevice(), texture->image, nullptr);
        vkFreeMemory(device->getCDevice(), texture->image.memory, nullptr);

        vkDestroyDescriptorSetLayout(device->getCDevice(), descriptor.setLayout, nullptr);

        vertexBuffer.reset();
        indexBuffer.reset();
        // vkDestroyBuffer(device->getCDevice(), vertexBuffer->buffer, nullptr);
        // vkFreeMemory(device->getCDevice(), vertexBuffer->memory, nullptr);

        // vkDestroyBuffer(device->getCDevice(), indexBuffer->buffer, nullptr);
        // vkFreeMemory(device->getCDevice(), indexBuffer->memory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device->getCDevice(), imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device->getCDevice(), renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device->getCDevice(), inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device->getCDevice(), context->graphicsCommandPool, nullptr);
        vkDestroyCommandPool(device->getCDevice(), context->transferCommandPool, nullptr);

        vkDestroyDevice(device->getCDevice(), nullptr);

        vkDestroySurfaceKHR(context->instance.get(), swapchain.surface, nullptr);
        
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
}