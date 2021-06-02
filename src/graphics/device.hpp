#pragma once

#include "commandpool.hpp"
#include "instance.hpp"
#include "queue.hpp"

#include <assert.h>
#include <optional>
#include <set>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace vkg
{

struct SwapchainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    SwapchainSupportDetails(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
    {
        capabilities = device.getSurfaceCapabilitiesKHR(surface);
        formats = device.getSurfaceFormatsKHR(surface);
        presentModes = device.getSurfacePresentModesKHR(surface);
    };
};

/// @brief Wrapper around  a vulkan logical device.
class Device
{
  public:
    // Physical device we're associated to.
    vk::PhysicalDevice m_physical;

    // Wrapped logical vulkan device.
    vk::UniqueDevice m_logical;

    vk::PhysicalDeviceMemoryProperties m_memoryProperties;

    std::vector<const char*> m_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;
    std::unordered_map<std::type_index, vk::UniqueDescriptorSetLayout> m_descriptorSetLayouts;

    struct
    {
        Queue graphics;
        Queue present;
        Queue transfer;
    } m_queues;

    // TODO: Multi threaded rendering would require different pools for each thread
    struct
    {
        CommandPool graphics;
        CommandPool transfer;
    } m_commandpools;

    // TODO: Multi-threaded requires a different pool per thread
    vk::UniqueDescriptorPool m_descriptorPool;

    Device();
    Device(const vk::SurfaceKHR& surface);

    SwapchainSupportDetails GetSwapchainSupport(const vk::SurfaceKHR& surface);

    static bool CheckDeviceExtensionsSupport(const vk::PhysicalDevice& dev, std::vector<const char*> requiredExtensions);

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format FindDepthFormat();
    vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    /// @brief Check if the device supports blitting to linear images.
    // TODO: Make more versatile
    bool SupportsBlittingToLinearImages();

    /// @brief Add a name to a vulkan object for debugging purposes.
    /// @param object: vulkan object to add a debug name to
    /// @param name: debug name
    template <class T>
    void SetDebugUtilsObjectName(T object, std::string name)
    {
        vk::DebugUtilsObjectNameInfoEXT debugName{
            .objectType = object.objectType,
            .objectHandle = (uint64_t) (typename T::CType) object,
            .pObjectName = name.c_str(),
        };

        m_logical->setDebugUtilsObjectNameEXT(debugName, Instance::GetDispatchLoaderDynamic());
    }

    inline vk::SampleCountFlagBits GetMSAASamples() const { return m_msaaSamples; }
    inline vk::Device& GetVkDevice() { return m_logical.get(); }
    inline vk::PhysicalDevice& GetVkPhysicalDevice() { return m_physical; }
    inline vk::FormatProperties GetFormatProperties(vk::Format format) { return m_physical.getFormatProperties(format); }

    // Queues getters
    inline Queue& GetGraphicsQueue() { return m_queues.graphics; }
    inline Queue& GetPresentQueue() { return m_queues.present; }
    inline Queue& GetTransferQueue() { return m_queues.transfer; }

    // Command Pool getters
    inline CommandPool& GetTransferCommandPool() { return m_commandpools.transfer; };
    inline CommandPool& GetGraphicsCommandPool() { return m_commandpools.graphics; };

    inline const vk::PhysicalDeviceProperties GetPhysicalDeviceProperties() const { return m_physical.getProperties(); }
    inline vk::DescriptorPool GetDescriptorPool() { return m_descriptorPool.get(); };

    /// @brief Allocates a single descriptor set from this device's default pool.
    /// @todo Handle multi-threading.
    template <typename T>
    vk::UniqueDescriptorSet AllocateDescriptorSet()
    {
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = m_descriptorPool.get();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &GetDescriptorSetLayout<T>();

        return std::move(m_logical->allocateDescriptorSetsUnique(allocInfo)[0]);
    }

    /// @brief Get the descriptor set layout associated with type T. If the layout doesn't exist it will be created.
    /// Aditionnaly registers the layout to enable automatic destruction.
    template <typename T>
    vk::DescriptorSetLayout& GetDescriptorSetLayout()
    {
        std::type_index type_index = std::type_index(typeid(T));
        auto iter = m_descriptorSetLayouts.find(type_index);

        // Create the descriptor set layout if it doesn't exist yet.
        if (iter == m_descriptorSetLayouts.end())
        {
            auto bindings = T::GetDescriptorSetLayoutBindings();
            vk::DescriptorSetLayoutCreateInfo createInfo = {
                .bindingCount = static_cast<uint32_t>(bindings.size()),
                .pBindings = bindings.data(),
            };

            auto layout = m_logical->createDescriptorSetLayoutUnique(createInfo);
            iter = m_descriptorSetLayouts.emplace(type_index, std::move(layout)).first;
        }
        return iter->second.get();
    }

  private:
    void CreateLogicalDevice(const vk::SurfaceKHR& surface);
    vk::PhysicalDevice PickPhysicalDevice(const vk::SurfaceKHR& surface);
    vk::SampleCountFlagBits GetMaxUsableSampleCount();

    static bool IsDeviceSuitable(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface, std::vector<const char*> requiredExtensions);
};
} // namespace vkg