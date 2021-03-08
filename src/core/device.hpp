#pragma once

#include "commandpool.hpp"
#include "instance.hpp"
#include "queue.hpp"

#include <assert.h>
#include <optional>
#include <set>
#include <vulkan/vulkan.hpp>

namespace core
{

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
    }
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

struct SwapchainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class Device
{
  public:
    vk::PhysicalDevice physical;
    vk::UniqueDevice logical;

    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceFeatures features;
    core::QueueFamilyIndices queueFamilyIndices;
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties;
    vk::PhysicalDeviceMemoryProperties memoryProperties;

    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;

    struct
    {
        core::Queue graphics;
        core::Queue present;
        core::Queue transfer;
    } queues;

    struct
    {
        core::CommandPool graphics;
        core::CommandPool transfer;
    } commandpools;

    Device();
    Device(const vk::UniqueSurfaceKHR& surface);

    SwapchainSupportDetails getSwapchainSupport(const vk::UniqueSurfaceKHR& surface);

    QueueFamilyIndices getQueueFamilyIndices();

    static bool checkDeviceExtensionsSupport(const vk::PhysicalDevice& dev, std::vector<const char*> requiredExtensions);

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format findDepthFormat();
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    // Check if the device supports blitting to linear images
    // TODO: Make more versatile
    bool supportsBlittingToLinearImages();

    /// @brief Add a name to a vulkan object for debugging purposes.
    /// @param object: vulkan object to add a debug name to
    /// @param name: debug name
    template <class T>
    void setDebugUtilsObjectName(T object, std::string name)
    {
        vk::DebugUtilsObjectNameInfoEXT debugName{object.objectType, (uint64_t)(typename T::CType) object, name.c_str()};
        logical->setDebugUtilsObjectNameEXT(debugName, core::Instance::Singleton().m_dispatchLoaderDynamic);
    }

  private:
    void createLogicalDevice(const vk::UniqueSurfaceKHR& surface, const bool enableValidationLayers = true);
    void createCommandPools();
    void initProperties();

    static SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice& device, const vk::UniqueSurfaceKHR& surface);
    vk::SampleCountFlagBits getMaxUsableSampleCount();

    /* TODO: This is static but only used internally. We may aswell remove the static part and assert that physical device has been picked. */
    static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, const vk::UniqueSurfaceKHR& surface);

    static bool isDeviceSuitable(const vk::PhysicalDevice& device, const vk::UniqueSurfaceKHR& surface, std::vector<const char*> requiredExtensions);
    vk::PhysicalDevice pickPhysicalDevice(const vk::UniqueSurfaceKHR& surface);
};
} // namespace core