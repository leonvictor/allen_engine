#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <utils/singleton.hpp>

namespace vkg
{

/// @brief Instance stores application-wide parameters and info, as well as the debugging utilities.
/// It is a singleton (use Instance::)
class Instance : private ISingleton<Instance>
// class Instance
{
    friend class ISingleton<Instance>;
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
    static bool CheckValidationLayersSupport(const std::vector<const char*> validationLayers);

    /// @brief Check if the instance supports all the requested extensions.
    static bool CheckExtensionSupport(std::vector<const char*> extensions);

  public:
    /// @brief Create the application wide instance. Should be called once at program startup.
    static void Create();

    static bool IsInitialized()
    {
        return Singleton().m_status == State::Initialized;
    }

    /// @brief Get the wrapped vulkan instance.
    static vk::Instance& Get() { return Singleton().m_vkInstance.get(); }

    static void RequestExtension(const char* extension)
    {
        Singleton().m_requestedExtensions.push_back(extension);
    }

    static void RequestExtensions(std::vector<const char*> extensions)
    {
        auto& inst = Singleton();
        inst.m_requestedExtensions.insert(inst.m_requestedExtensions.end(), extensions.begin(), extensions.end());
    }

    static const std::vector<const char*> GetValidationLayers()
    {
        return Singleton().m_validationLayers;
    }

    static const vk::DispatchLoaderDynamic& GetDispatchLoaderDynamic()
    {
        auto& single = Singleton();
        assert(single.IsInitialized());
        return single.m_dispatchLoaderDynamic;
    }

    static bool ValidationLayersEnabled() { return Singleton().ValidationLayersEnabled; }
};
} // namespace vkg

extern template class ISingleton<vkg::Instance>;