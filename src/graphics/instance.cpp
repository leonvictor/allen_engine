#include "instance.hpp"
#include <iostream>

namespace vkg
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

void Instance::Create()
{

    auto& inst = Singleton();
    if (!CheckExtensionSupport(inst.m_requestedExtensions))
    {
        throw std::runtime_error("Vulkan extension requested but not available.");
    }

    if (inst.m_validationLayersEnabled && !CheckValidationLayersSupport(inst.m_validationLayers))
    {
        throw std::runtime_error("Validation layers requested but not available.");
    }

    if (inst.m_validationLayersEnabled)
    {
        // TODO: Move to ValidationExtension
        inst.m_requestedExtensions.push_back("VK_EXT_debug_utils");
    }

    // Populate the ApplicationInfo struct. Optionnal but may provide useful info to the driver
    vk::ApplicationInfo appInfo{
        .pApplicationName = "Not-so-poopy game editor",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Not-so-poopy engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    vk::InstanceCreateInfo iCreateInfo;
    iCreateInfo.pApplicationInfo = &appInfo,
    iCreateInfo.enabledExtensionCount = static_cast<uint32_t>(inst.m_requestedExtensions.size());
    iCreateInfo.ppEnabledExtensionNames = inst.m_requestedExtensions.data();

    // Enable validation layers if needed
    vk::DebugUtilsMessengerCreateInfoEXT dCreateInfo;
    if (inst.m_validationLayersEnabled)
    {
        iCreateInfo.enabledLayerCount = static_cast<uint32_t>(inst.m_validationLayers.size());
        iCreateInfo.ppEnabledLayerNames = inst.m_validationLayers.data();

        // Create the debugger
        dCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
        dCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
        //TODO: Is there something like "all flags" ?
        dCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT) DebugCallback;

        iCreateInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &dCreateInfo;
    }
    else
    {
        iCreateInfo.enabledLayerCount = 0;
        iCreateInfo.pNext = nullptr;
    }

    inst.m_vkInstance = vk::createInstanceUnique(iCreateInfo);

    if (inst.m_validationLayersEnabled)
    {
        inst.m_dispatchLoaderDynamic = vk::DispatchLoaderDynamic{inst.m_vkInstance.get(), vkGetInstanceProcAddr};
        inst.m_debugMessenger = inst.m_vkInstance->createDebugUtilsMessengerEXTUnique(
            dCreateInfo,
            nullptr, inst.m_dispatchLoaderDynamic);
    }

    inst.m_status = State::Initialized;
}
}; // namespace vkg