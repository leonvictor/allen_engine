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
    vk::UniqueInstance m_vkInstance; /// Wrapped vulkan instance
    vk::DispatchLoaderDynamic m_dispatchLoaderDynamic;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_debugMessenger;

    bool CheckValidationLayersSupport(const std::vector<const char*> validationLayers) const;

  public:
    /// @brief Create the application wide instance. Should be called once at program startup.
    /// TODO: params
    void Create(std::vector<const char*> requiredExtensions, bool enableValidationLayers, std::vector<const char*> validationLayers);

    /// @brief Get the singleton instance associated to this app.
    static Instance& Singleton()
    {
        static Instance single;
        return single;
    }

    /// @brief Get the wrapped vulkan instance
    inline vk::Instance& Get()
    {
        return m_vkInstance.get();
    }
};
} // namespace core