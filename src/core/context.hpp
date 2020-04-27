#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "device.hpp"
#include "commandpool.cpp"
#include "buffer.cpp"

#include <memory>
#include <iostream>

namespace core {
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection; 
    };
    
    class Context {
    public:
        vk::UniqueInstance instance;
        std::shared_ptr<core::Device> device;

        core::CommandPool graphicsCommandPool;
        core::CommandPool transferCommandPool;

        vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debugMessenger;

        // const std::string TEXTURE_PATH = "assets/textures/camel.jpg"; // TODO: Nope ! Chuck testa

        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation",
        };

        bool enableValidationLayers = true; // TODO: Move that somewhere else (global config)

        Context() {
            createInstance();
            setupDebugMessenger();
        }

        void createCommandPools(std::shared_ptr<core::Device> device) {
            this-> device = device; // TODO: This HAS to be somewhere else.
            graphicsCommandPool = core::CommandPool(device, device->queueFamilyIndices.graphicsFamily.value());
            transferCommandPool = core::CommandPool(device, device->queueFamilyIndices.transferFamily.value(), vk::CommandPoolCreateFlagBits::eTransient);
        }

        // TODO: Where does this belong ?
        /* Requires a ref to : command pool, queue
         * Context is the usual suspect *but* it would be better if buffers had no knowledge of the context they're part of
         * CommandPools on the other hand could hold a ref to the queue they're attached to.
         */
        void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
            auto commandBuffer = transferCommandPool.beginSingleTimeCommands();
            
            vk::BufferImageCopy copy;
            copy.bufferOffset = 0;
            copy.bufferRowLength = 0;
            copy.bufferImageHeight = 0;
            copy.imageExtent = vk::Extent3D{width, height, 1};
            copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount = 1;
            copy.imageSubresource.mipLevel = 0;
            copy.imageOffset = vk::Offset3D{0, 0, 0};

            commandBuffer[0].copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &copy);

            transferCommandPool.endSingleTimeCommands(commandBuffer, device->transferQueue);
        }

        void copyBuffer(const core::Buffer& srcBuffer, const core::Buffer& dstBuffer, vk::DeviceSize size) {
            auto commandBuffers = transferCommandPool.beginSingleTimeCommands();

            vk::BufferCopy copyRegion;
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = size;

            commandBuffers[0].copyBuffer(srcBuffer.buffer, dstBuffer.buffer, copyRegion);
            transferCommandPool.endSingleTimeCommands(commandBuffers, device->transferQueue);
    }

    private:
        void createInstance(){
            if (enableValidationLayers && !checkValidationLayersSupport()){
                throw std::runtime_error("validation layers requested but not available");
            }

            // Populate the ApplicationInfo struct. Optionnal but may provide useful info to the driver
            vk::ApplicationInfo appInfo(
                "Not-so-poopy game editor",
                VK_MAKE_VERSION(1, 0, 0),
                "Not-so-poopy engine",
                VK_MAKE_VERSION(1, 0, 0),
                VK_API_VERSION_1_0
            );

            auto extensions = getRequiredExtensions();
            vk::InstanceCreateInfo iCreateInfo;
            
            iCreateInfo.pApplicationInfo = &appInfo,
            iCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            iCreateInfo.ppEnabledExtensionNames = extensions.data();

            // Enable validation layers if needed
            if (enableValidationLayers){
                iCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                iCreateInfo.ppEnabledLayerNames = validationLayers.data();

                vk::DebugUtilsMessengerCreateInfoEXT dCreateInfo = getDebugMessengerCreateInfo();
                iCreateInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &dCreateInfo;
            } else {
                iCreateInfo.enabledLayerCount = 0; // Validation layers. None for now
                iCreateInfo.pNext = nullptr;
            }
            
            // TODO: There HAS to be a cleaner way
            auto inst = vk::createInstanceUnique(iCreateInfo);
            instance = std::move(inst);
        };

        bool checkValidationLayersSupport() {
            std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

            for (const char* layerName : validationLayers) {
                bool layerFound = false;
                for (const auto& layerProperties: availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound){
                    return false;
                }
            }
            return true;
        }

        std::vector<const char*> getRequiredExtensions() {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;

            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // GLFW function that return the extensions it needs
            
            
            // Check if all extensions required by GLFW are available
            std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
            for (uint32_t i{}; i<glfwExtensionCount; ++i) {
                bool extensionFound = false;
                for (const auto& extension: availableExtensions) {
                    if (strcmp(extension.extensionName, glfwExtensions[i])) {
                        extensionFound = true;
                        break;
                    }
                }
                if(!extensionFound){
                    throw std::runtime_error("Missing extension required by GLFW.");
                }
            }

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount); // ??

            if (enableValidationLayers) {
                extensions.push_back("VK_EXT_debug_utils");
            }

            return extensions;
        }

        vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo() {
            vk::DebugUtilsMessengerCreateInfoEXT dCreateInfo;
            dCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
            dCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
            //TODO: Is there something like "all flags" ?
            dCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT) debugCallback;
            return dCreateInfo;
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            vk::DebugUtilsMessageTypeFlagsEXT messageType,
            const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData
        ) {
            std::cerr << "validation layer: " << pCallbackData -> pMessage << std::endl;
            return VK_FALSE; // Should the vulkan call that triggered the validation be aborted ?
        }

        void setupDebugMessenger() {
            if (!enableValidationLayers) return;

            debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
                getDebugMessengerCreateInfo(),
                nullptr,
                vk::DispatchLoaderDynamic{ *instance , vkGetInstanceProcAddr}
                );
        }
    };   
};