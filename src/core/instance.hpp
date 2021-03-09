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
    enum State
    {
        Uninitialized,
        Initialized
    };

    State m_status = State::Uninitialized;

    vk::UniqueInstance m_vkInstance; // Wrapped vulkan instance
    vk::DispatchLoaderDynamic m_dispatchLoaderDynamic;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_debugMessenger;

    std::vector<const char*> m_requestedExtensions;

    bool CheckValidationLayersSupport(const std::vector<const char*> validationLayers) const;

    /// @brief Check if an instance supports all the requested extensions.
    static bool CheckExtensionSupport(std::vector<const char*> extensions);

  public:
    /// @brief Create the application wide instance. Should be called once at program startup.
    /// TODO: params
    void Create(bool enableValidationLayers, std::vector<const char*> validationLayers);

    bool IsInitialized() const
    {
        return m_status == State::Initialized;
    }

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

    inline void RequestExtension(const char* extension)
    {
        m_requestedExtensions.push_back(extension);
    }

    inline void RequestExtensions(std::vector<const char*> extensions)
    {
        m_requestedExtensions.insert(m_requestedExtensions.begin(), extensions.begin(), extensions.end());
    }
};
} // namespace core