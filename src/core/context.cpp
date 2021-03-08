// #pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "device.hpp"
#include "instance.hpp"

#include "context.hpp"
#include <iostream>
#include <memory>

namespace core
{

Context::Context(GLFWwindow* window)
{
    auto requiredExtensions = getRequiredExtensions();
    core::Instance::Singleton().Create(requiredExtensions, enableValidationLayers, validationLayers);

    createSurface(window);
    device = std::make_shared<core::Device>(surface);
}

void Context::createSurface(GLFWwindow* window)
{
    VkSurfaceKHR pSurface;
    if (glfwCreateWindowSurface((VkInstance) core::Instance::Singleton().Get(), window, nullptr, &pSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface.");
    }

    surface = vk::UniqueSurfaceKHR(pSurface, core::Instance::Singleton().Get());
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
}; // namespace core