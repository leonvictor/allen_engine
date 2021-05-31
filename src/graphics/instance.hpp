#pragma once

#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace vkg
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

#ifdef NDEBUG
    const bool ValidationLayersEnabled = false;
#else
    const bool ValidationLayersEnabled = true;
#endif

    State m_status = State::Uninitialized;

    vk::UniqueInstance m_vkInstance; // Wrapped vulkan instance
    vk::DispatchLoaderDynamic m_dispatchLoaderDynamic;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_debugMessenger;

    std::vector<const char*> m_requestedExtensions = {};

    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    /// @brief Check if the instance supports the validation layers.
    static bool CheckValidationLayersSupport(const std::vector<const char*> validationLayers);

    /// @brief Check if the instance supports all the requested extensions.
    static bool CheckExtensionSupport(std::vector<const char*> extensions);

  public:
    /// @brief Create the application wide instance. Should be called once at program startup.
    void Create();

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

    /// @brief Get the wrapped vulkan instance.
    inline vk::Instance& Get() { return m_vkInstance.get(); }

    inline void RequestExtension(const char* extension)
    {
        m_requestedExtensions.push_back(extension);
    }

    inline void RequestExtensions(std::vector<const char*> extensions)
    {
        m_requestedExtensions.insert(m_requestedExtensions.end(), extensions.begin(), extensions.end());
    }

    inline const std::vector<const char*> GetValidationLayers() const
    {
        return m_validationLayers;
    }
};
} // namespace vkg