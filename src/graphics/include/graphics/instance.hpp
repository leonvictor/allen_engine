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
    vk::UniqueInstance m_vkInstance; // Wrapped vulkan instance
    vk::DispatchLoaderDynamic m_dispatchLoaderDynamic;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> m_debugMessenger;

    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    #ifdef NDEBUG
    const bool m_validationLayersEnabled = false;
#else
    const bool m_validationLayersEnabled = true;
#endif

    /// @brief Check if the instance supports the validation layers.
    bool CheckValidationLayersSupport(const std::vector<const char*> validationLayers);

    /// @brief Check if the instance supports all the requested extensions.
    bool CheckExtensionSupport(std::vector<const char*> extensions);

  public:
    /// @brief Create the application wide instance. Should be called once at program startup.
    void Initialize(std::vector<const char*>& requestedExtensions);

    /// @brief Get the wrapped vulkan instance.
    const vk::Instance& GetVkInstance() const { return m_vkInstance.get(); }
    const std::vector<const char*>& GetValidationLayers() const { return m_validationLayers; }
    const vk::DispatchLoaderDynamic& GetDispatchLoaderDynamic() const { return m_dispatchLoaderDynamic; }
    bool ValidationLayersEnabled() const { return m_validationLayersEnabled; }
};
} // namespace aln::vkg