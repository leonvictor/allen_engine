#include "instance.hpp"
#include <iostream>

namespace core
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

bool Instance::CheckValidationLayersSupport(const std::vector<const char*> validationLayers) const
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

void Instance::Create(std::vector<const char*> requiredExtensions, bool enableValidationLayers, std::vector<const char*> validationLayers)
{
    if (enableValidationLayers && !CheckValidationLayersSupport(validationLayers))
    {
        throw std::runtime_error("validation layers requested but not available");
    }

    // Populate the ApplicationInfo struct. Optionnal but may provide useful info to the driver
    vk::ApplicationInfo appInfo(
        "Not-so-poopy game editor",
        VK_MAKE_VERSION(1, 0, 0),
        "Not-so-poopy engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_2);

    // auto extensions = getRequiredExtensions();

    vk::InstanceCreateInfo iCreateInfo;
    iCreateInfo.pApplicationInfo = &appInfo,
    iCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    iCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // Enable validation layers if needed
    vk::DebugUtilsMessengerCreateInfoEXT dCreateInfo;
    if (enableValidationLayers)
    {
        iCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        iCreateInfo.ppEnabledLayerNames = validationLayers.data();

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

    m_vkInstance = vk::createInstanceUnique(iCreateInfo);

    if (enableValidationLayers)
    {
        m_dispatchLoaderDynamic = vk::DispatchLoaderDynamic{m_vkInstance.get(), vkGetInstanceProcAddr};
        m_debugMessenger = m_vkInstance->createDebugUtilsMessengerEXTUnique(
            dCreateInfo,
            nullptr, m_dispatchLoaderDynamic);
    }
}
};