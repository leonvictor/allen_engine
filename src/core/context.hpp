#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "buffer.cpp"
#include "commandpool.cpp"
#include "device.hpp"

#include <iostream>
#include <memory>

namespace core
{
struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 cameraPos;
};

class Context
{
  public:
    vk::UniqueInstance instance;
    std::shared_ptr<core::Device> device;
    vk::SurfaceKHR surface;

    core::CommandPool graphicsCommandPool;
    core::CommandPool transferCommandPool;

    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debugMessenger;

    // const std::string TEXTURE_PATH = "assets/textures/camel.jpg"; // TODO: Nope ! Chuck testa

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    bool enableValidationLayers = true; // TODO: Move that somewhere else (global config)

    Context(GLFWwindow* window)
    {
        createInstance();
        createSurface(window);
        device = std::make_shared<core::Device>(instance.get(), surface);
        createCommandPools();
        setupDebugMessenger();
    }

    void createCommandPools()
    {
        graphicsCommandPool = core::CommandPool(device, device->queueFamilyIndices.graphicsFamily.value(), vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        transferCommandPool = core::CommandPool(device, device->queueFamilyIndices.transferFamily.value(), vk::CommandPoolCreateFlagBits::eTransient);
    }

    void destroy()
    {
        device->logicalDevice.destroyCommandPool(graphicsCommandPool);
        device->logicalDevice.destroyCommandPool(transferCommandPool);
        device->logicalDevice.destroy();

        instance->destroySurfaceKHR(surface);
    }
    
  private:
    void createInstance()
    {
        if (enableValidationLayers && !checkValidationLayersSupport())
        {
            throw std::runtime_error("validation layers requested but not available");
        }

        // Populate the ApplicationInfo struct. Optionnal but may provide useful info to the driver
        vk::ApplicationInfo appInfo(
            "Not-so-poopy game editor",
            VK_MAKE_VERSION(1, 0, 0),
            "Not-so-poopy engine",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_0);

        auto extensions = getRequiredExtensions();
        vk::InstanceCreateInfo iCreateInfo;

        iCreateInfo.pApplicationInfo = &appInfo,
        iCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        iCreateInfo.ppEnabledExtensionNames = extensions.data();

        // Enable validation layers if needed
        if (enableValidationLayers)
        {
            iCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            iCreateInfo.ppEnabledLayerNames = validationLayers.data();

            vk::DebugUtilsMessengerCreateInfoEXT dCreateInfo = getDebugMessengerCreateInfo();
            iCreateInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &dCreateInfo;
        }
        else
        {
            iCreateInfo.enabledLayerCount = 0;
            iCreateInfo.pNext = nullptr;
        }

        instance = vk::createInstanceUnique(iCreateInfo);
    };

    void createSurface(GLFWwindow* window)
    {
        VkSurfaceKHR pSurface = VkSurfaceKHR(surface);
        VkInstance pInstance = (VkInstance) instance.get();
        if (glfwCreateWindowSurface(pInstance, window, nullptr, &pSurface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface.");
        }

        surface = vk::SurfaceKHR(pSurface);
    }

    bool checkValidationLayersSupport()
    {
        std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }
        return true;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // GLFW function that return the extensions it needs

        // Check if all extensions required by GLFW are available
        std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
        for (uint32_t i{}; i < glfwExtensionCount; ++i)
        {
            bool extensionFound = false;
            for (const auto& extension : availableExtensions)
            {
                if (strcmp(extension.extensionName, glfwExtensions[i]))
                {
                    extensionFound = true;
                    break;
                }
            }
            if (!extensionFound)
            {
                throw std::runtime_error("Missing extension required by GLFW.");
            }
        }

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount); // ??

        if (enableValidationLayers)
        {
            extensions.push_back("VK_EXT_debug_utils");
        }

        return extensions;
    }

    vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo()
    {
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
        void* pUserData)
    {
        if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        {
            std::cout << "Warning: " << pCallbackData->messageIdNumber << ":" << pCallbackData->pMessageIdName << ":" << pCallbackData->pMessage << std::endl;
        }
        else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        {
            std::cerr << "Error: " << pCallbackData->messageIdNumber << ":" << pCallbackData->pMessageIdName << ":" << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
            getDebugMessengerCreateInfo(),
            nullptr,
            vk::DispatchLoaderDynamic{*instance, vkGetInstanceProcAddr});
    }
};
}; // namespace core