#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace aln::vkg
{

/// @brief Instance stores application-wide parameters and info, as well as the debugging utilities.

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
    const bool m_validationLayersEnabled = false;
#else
    const bool m_validationLayersEnabled = true;
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
    bool CheckValidationLayersSupport(const std::vector<const char*> validationLayers);

    /// @brief Check if the instance supports all the requested extensions.
    bool CheckExtensionSupport(std::vector<const char*> extensions);

  public:
    /// @brief Create the application wide instance. Should be called once at program startup.
    void Create();

    bool IsInitialized() const
    {
        return m_status == State::Initialized;
    }

    /// @brief Get the wrapped vulkan instance.
    const vk::Instance& GetVkInstance() const { return m_vkInstance.get(); }

    void RequestExtension(const char* extension)
    {
        m_requestedExtensions.push_back(extension);
    }

    void RequestExtensions(std::vector<const char*> extensions)
    {
        m_requestedExtensions.insert(m_requestedExtensions.end(), extensions.begin(), extensions.end());
    }

    const std::vector<const char*> GetValidationLayers() const
    {
        return m_validationLayers;
    }

    const vk::DispatchLoaderDynamic& GetDispatchLoaderDynamic()
    {
        assert(IsInitialized());
        return m_dispatchLoaderDynamic;
    }

    bool ValidationLayersEnabled() const { return m_validationLayersEnabled; }
};
} // namespace aln::vkg