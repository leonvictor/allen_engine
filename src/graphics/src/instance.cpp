#include "instance.hpp"
#include <iostream>

namespace aln::vkg
{
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
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

bool Instance::CheckValidationLayersSupport(const std::vector<const char*> validationLayers)
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

bool Instance::CheckExtensionSupport(std::vector<const char*> extensions)
{
    // Check if all extensions required by GLFW are available
    std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < extensions.size(); ++i)
    {
        bool extensionFound = false;
        for (const auto& extension : availableExtensions)
        {
            if (strcmp(extension.extensionName, extensions[i]))
            {
                extensionFound = true;
                break;
            }
        }

        if (!extensionFound)
        {
            return false;
        }
    }
    return true;
}

void Instance::Initialize(std::vector<const char*>& requestedExtensions)
{
    if (!CheckExtensionSupport(requestedExtensions))
    {
        throw std::runtime_error("Vulkan extension requested but not available.");
    }

    if (m_validationLayersEnabled && !CheckValidationLayersSupport(m_validationLayers))
    {
        throw std::runtime_error("Validation layers requested but not available.");
    }

    if (m_validationLayersEnabled)
    {
        requestedExtensions.push_back("VK_EXT_debug_utils");
    }

    // Populate the ApplicationInfo struct. Optionnal but may provide useful info to the driver
    vk::ApplicationInfo appInfo = {
        .pApplicationName = "Allen Game Editor",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "AllenEngine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    vk::InstanceCreateInfo instanceCreateInfo = {
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size()),
        .ppEnabledExtensionNames = requestedExtensions.data(),
    };

    // Enable validation layers if needed
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo;
    if (m_validationLayersEnabled)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();

        // Create the debugger
        debugUtilsMessengerCreateInfo = {
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            .pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT) DebugCallback,
        };
        instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
    }

    m_vkInstance = vk::createInstanceUnique(instanceCreateInfo);

    if (m_validationLayersEnabled)
    {
        m_dispatchLoaderDynamic = vk::DispatchLoaderDynamic{m_vkInstance.get(), vkGetInstanceProcAddr};
        m_debugMessenger = m_vkInstance->createDebugUtilsMessengerEXTUnique(
            debugUtilsMessengerCreateInfo,
            nullptr, m_dispatchLoaderDynamic);
    }
}
}; // namespace aln::vkg