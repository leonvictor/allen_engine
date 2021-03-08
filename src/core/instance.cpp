#pragma once

#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace core
{

/// @brief Instance stores application-wide parameters and info, as well as the debugging utilities.
/// It is a singleton (use Instance::Singleton())
class Instance
{
    friend class Device;

  private:
    vk::UniqueInstance m_vkInstance;

    vk::DispatchLoaderDynamic m_dispatchLoaderDynamic;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_debugMessenger;

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

    bool CheckValidationLayersSupport(std::vector<const char*> validationLayers) const
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

  public:
    /// @brief Create the application wide instance. Should be called once at program startup.
    /// TODO: params
    void Create(std::vector<const char*> requiredExtensions, bool enableValidationLayers, std::vector<const char*> validationLayers)
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

    /// @brief Get the singleton instance associated to this app.
    static Instance& Singleton()
    {
        static Instance single;
        return single;
    }

    /// @brief Get the wrapped vulkan instance
    vk::Instance& Get()
    {
        return m_vkInstance.get();
    }
};
} // namespace core