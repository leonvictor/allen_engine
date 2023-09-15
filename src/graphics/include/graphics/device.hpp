#pragma once

#include "commandpool.hpp"
#include "descriptor_allocator.hpp"
#include "instance.hpp"
#include "queue.hpp"

#include <assert.h>
#include <optional>
#include <set>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace aln::vkg
{

struct SwapchainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    Vector<vk::SurfaceFormatKHR> formats;
    Vector<vk::PresentModeKHR> presentModes;

    SwapchainSupportDetails(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
    {
        capabilities = device.getSurfaceCapabilitiesKHR(surface);
        
        uint32_t formatsCount;
        device.getSurfaceFormatsKHR(surface, &formatsCount, nullptr);
        formats.resize(formatsCount);
        device.getSurfaceFormatsKHR(surface, &formatsCount, formats.data());

        uint32_t presentModesCount;
        device.getSurfacePresentModesKHR(surface, &presentModesCount, nullptr);
        presentModes.resize(presentModesCount);
        device.getSurfacePresentModesKHR(surface, &presentModesCount, presentModes.data());
    };
};

/// @brief Wrapper around  a vulkan logical device.
class Device
{
  private:
    vkg::Instance* m_pInstance;

    // Physical device we're associated to.
    vk::PhysicalDevice m_physical;

    // Wrapped logical vulkan device.
    vk::UniqueDevice m_logical;

    vk::PhysicalDeviceMemoryProperties m_memoryProperties;
    vk::PhysicalDeviceProperties m_gpuProperties;

    Vector<const char*> m_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;

    std::unordered_map<std::type_index, vk::UniqueDescriptorSetLayout> m_descriptorSetLayoutsCache;
    DescriptorAllocator m_descriptorAllocator;

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

    void CreateLogicalDevice(const vk::SurfaceKHR& surface);
    vk::PhysicalDevice PickPhysicalDevice(const vk::SurfaceKHR& surface);
    vk::SampleCountFlagBits GetMaxUsableSampleCount();

    static bool IsDeviceSuitable(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface, Vector<const char*> requiredExtensions);

  public:
    Device(){};
    void Initialize(vkg::Instance* pInstance, const vk::SurfaceKHR& surface);

    SwapchainSupportDetails GetSwapchainSupport(const vk::SurfaceKHR& surface);

    static bool CheckDeviceExtensionsSupport(const vk::PhysicalDevice& dev, Vector<const char*> requiredExtensions);

    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format FindDepthFormat();
    vk::Format FindSupportedFormat(const Vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    /// @brief Pad a given size to meet the device's alignment requirements.
    /// @return: Adjusted size
    /// @note https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
    size_t PadUniformBufferSize(size_t originalSize);

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

        m_logical->setDebugUtilsObjectNameEXT(debugName, m_pInstance->GetDispatchLoaderDynamic());
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
    inline vk::DescriptorPool& GetDescriptorPool() { return m_descriptorAllocator.GetActivePool(); }
    inline const vkg::Instance* GetInstance() { return m_pInstance; }

    /// @brief Allocates a single descriptor set from this device's default pool.
    /// @todo Handle multi-threading.
    template <typename T>
    vk::UniqueDescriptorSet AllocateDescriptorSet()
    {
        return std::move(m_descriptorAllocator.Allocate(&GetDescriptorSetLayout<T>()));
    }

    vk::UniqueDescriptorSet AllocateDescriptorSet(const vk::DescriptorSetLayout* pDescriptorSetLayout)
    {
        return std::move(m_descriptorAllocator.Allocate(pDescriptorSetLayout));
    }

    /// @brief Get the descriptor set layout associated with type T. If the layout doesn't exist it will be created.
    /// Additionnaly caches the layout to enable automatic destruction.
    /// @todo Require T to be a derived type of Descriptible
    /// @todo Various types could use the same set layout. Rework to use a hash of the descriptor instead of this templated fn.
    template <typename T>
    vk::DescriptorSetLayout& GetDescriptorSetLayout()
    {
        std::type_index type_index = std::type_index(typeid(T));
        auto iter = m_descriptorSetLayoutsCache.find(type_index);

        // Create the descriptor set layout if it doesn't exist yet.
        if (iter == m_descriptorSetLayoutsCache.end())
        {
            // Requires that T implements the function
            auto bindings = T::GetDescriptorSetLayoutBindings();

            vk::DescriptorSetLayoutCreateInfo info;
            info.bindingCount = static_cast<uint32_t>(bindings.size());
            info.pBindings = bindings.data();

            auto layout = m_logical->createDescriptorSetLayoutUnique(info);
            SetDebugUtilsObjectName(layout.get(), typeid(T).name());

            iter = m_descriptorSetLayoutsCache.emplace(type_index, std::move(layout)).first;
        }
        return iter->second.get();
    }
};
} // namespace aln::vkg