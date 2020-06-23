// #pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "commandpool.cpp"
#include "device.hpp"

#include "context.hpp"
#include <iostream>
#include <memory>

namespace core
{

Context::Context(GLFWwindow* window)
{
    createInstance();
    createSurface(window);
    device = std::make_shared<core::Device>(instance, surface);
    createCommandPools();
    setupDebugMessenger();
}

void Context::createCommandPools()
{
    graphicsCommandPool = core::CommandPool(device, device->queueFamilyIndices.graphicsFamily.value(), vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    transferCommandPool = core::CommandPool(device, device->queueFamilyIndices.transferFamily.value(), vk::CommandPoolCreateFlagBits::eTransient);
}

void Context::destroy()
{
    // TODO: Remove when RAII is functionnal
    device->logical.get().destroyCommandPool(graphicsCommandPool.pool);
    device->logical.get().destroyCommandPool(transferCommandPool.pool);

    device->logical.get().destroy();
    instance->destroySurfaceKHR(surface.get());
}

void Context::createInstance()
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

void Context::createSurface(GLFWwindow* window)
{
    VkSurfaceKHR pSurface;
    VkInstance pInstance = (VkInstance) instance.get();
    if (glfwCreateWindowSurface(pInstance, window, nullptr, &pSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface.");
    }

    surface = vk::UniqueSurfaceKHR(pSurface);
}

bool Context::checkValidationLayersSupport()
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

std::vector<const char*> Context::getRequiredExtensions()
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

vk::DebugUtilsMessengerCreateInfoEXT Context::getDebugMessengerCreateInfo()
{
    vk::DebugUtilsMessengerCreateInfoEXT dCreateInfo;
    dCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
    dCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    //TODO: Is there something like "all flags" ?
    dCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT) debugCallback;
    return dCreateInfo;
}

void Context::setupDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
        getDebugMessengerCreateInfo(),
        nullptr,
        vk::DispatchLoaderDynamic{*instance, vkGetInstanceProcAddr});
}
}; // namespace core