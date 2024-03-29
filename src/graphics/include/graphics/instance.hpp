#pragma once

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

#include <iostream>

namespace aln
{
/// @brief Instance stores application-wide parameters and info, as well as the debugging utilities.
class Instance
{
    friend class RenderEngine;

  private:
    vk::Instance m_instance; // Wrapped vulkan instance
    vk::DebugUtilsMessengerEXT m_debugMessenger;

    const Vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    #ifdef NDEBUG
    const bool m_validationLayersEnabled = false;
#else
    const bool m_validationLayersEnabled = true;
#endif

    /// @brief Check if the instance supports the validation layers.
    bool CheckValidationLayersSupport(const Vector<const char*> validationLayers);

    /// @brief Check if the instance supports all the requested extensions.
    bool CheckExtensionSupport(Vector<const char*> extensions);

  public:
    /// @brief Create the application wide instance. Should be called once at program startup.
    void Initialize(Vector<const char*>& requestedExtensions);
    void Shutdown();

    /// @brief Get the wrapped vulkan instance.
    const vk::Instance& GetVkInstance() const { return m_instance; }
    const Vector<const char*>& GetValidationLayers() const { return m_validationLayers; }
    bool ValidationLayersEnabled() const { return m_validationLayersEnabled; }
};
} // namespace aln