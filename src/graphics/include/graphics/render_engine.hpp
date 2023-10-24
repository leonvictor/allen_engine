#pragma once

#include "commandpool.hpp"
#include "descriptor_allocator.hpp"
#include "instance.hpp"
#include "queue.hpp"
#include "swapchain.hpp"

#include <common/containers/array.hpp>
#include <common/containers/hash_map.hpp>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <assert.h>
#include <typeindex>
#include <typeinfo>

namespace aln
{

class IWindow;

struct SwapchainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    Vector<vk::SurfaceFormatKHR> formats;
    Vector<vk::PresentModeKHR> presentModes;

    SwapchainSupportDetails(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface)
    {
        capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface).value;

        uint32_t formatsCount;
        physicalDevice.getSurfaceFormatsKHR(surface, &formatsCount, nullptr);
        formats.resize(formatsCount);
        physicalDevice.getSurfaceFormatsKHR(surface, &formatsCount, formats.data());

        uint32_t presentModesCount;
        physicalDevice.getSurfacePresentModesKHR(surface, &presentModesCount, nullptr);
        presentModes.resize(presentModesCount);
        physicalDevice.getSurfacePresentModesKHR(surface, &presentModesCount, presentModes.data());
    };
};

/// @brief Core rendering engine
/// @todo Use "unique" vk structs only when beneficial to avoid unecessary overhead
class RenderEngine
{
  private:
    static constexpr uint32_t FRAME_QUEUE_SIZE = 2;

    // Per-frame per-thread data
    /// @note // It's more efficient to reset command pools than reseting individual command buffers
    // We have 2x(queue types) pools:
    // - one for "transient" cbs, that must be done before we reuse the pool, i.e. rendering cbs
    // - one for "one time" cbs, that can cross frame boundaries and are reset when their execution is complete
    struct ThreadData
    {
        TransientCommandPool m_graphicsTransientCommandPool;
        TransientCommandPool m_transferTransientCommandPool;
        GraphicsQueuePersistentCommandPool m_graphicsPersistentCommandPool;
        TransferQueuePersistentCommandPool m_transferPersistentCommandPool;
    };

    // Per-frame in queue data
    struct FrameData
    {
        Vector<ThreadData> m_threadData;
        vk::Fence m_currentlyRendering;
    };

  private:
    // Vulkan objects
    vk::DynamicLoader m_dynamicLoader;
    Instance m_instance;
    vk::PhysicalDevice m_physical; // Physical pRenderEngine we're associated to.
    vk::Device m_logical;          // Wrapped logical vulkan pRenderEngine.

    // Disambiguation : Frame Queue refers to the frames we can be rendering at the same time.
    // It is not the same as swapchainImages, which represent the number of images the swapchain is able to provide
    // FrameQueueSize <= SwapchainImageCount
    Array<FrameData, FRAME_QUEUE_SIZE> m_frameData;

    Queue m_graphicsQueue;
    Queue m_presentQueue;
    Queue m_transferQueue;

    // TODO: Decouple so that we can draw to multiple windows
    IWindow* m_pWindow = nullptr;

    // Properties and capabilities
    vk::PhysicalDeviceMemoryProperties m_memoryProperties;
    vk::PhysicalDeviceProperties m_gpuProperties;
    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;
    Vector<const char*> m_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Descriptors
    HashMap<std::type_index, vk::DescriptorSetLayout, std::hash<std::type_index>> m_descriptorSetLayoutsCache;
    DescriptorAllocator m_descriptorAllocator;

    // Runtime
    uint32_t m_currentFrameIdx = 0;

  private:
    void CreateLogicalDevice();
    vk::PhysicalDevice PickPhysicalDevice();
    vk::SampleCountFlagBits GetMaxUsableSampleCount();
    static bool IsDeviceSuitable(const vk::PhysicalDevice& pRenderEngine, const vk::SurfaceKHR& surface, Vector<const char*> requiredExtensions);

  public:
    // -- Getters
    // TODO: Those are necessary for now because of some functionnality gravitating outside. Remove when possible !
    Instance* GetInstance() { return &m_instance; }
    vk::Device& GetVkDevice() { return m_logical; }
    vk::PhysicalDevice& GetVkPhysicalDevice() { return m_physical; }
    IWindow* GetWindow() { return m_pWindow; }

    // -- Lifetime
    void Initialize(IWindow* pWindow);
    void Shutdown();
    void WaitIdle() { m_logical.waitIdle(); }

    void StartFrame()
    {
        auto& frameCurrentlyRenderingFence = m_frameData[m_currentFrameIdx].m_currentlyRendering;
        assert(frameCurrentlyRenderingFence);
        // Ensure the current frame's previous render is finished then reset it
        // TODO: We could delay the reset until we're actually submitting
        m_logical.waitForFences(frameCurrentlyRenderingFence, vk::True, UINT64_MAX);
        m_logical.resetFences(frameCurrentlyRenderingFence);

        // Reset all command pools
        for (auto& threadData : m_frameData[m_currentFrameIdx].m_threadData)
        {
            threadData.m_graphicsTransientCommandPool.Reset();
            threadData.m_transferTransientCommandPool.Reset();
        }
    }

    void EndFrame()
    {
        m_currentFrameIdx = (m_currentFrameIdx + 1) % FRAME_QUEUE_SIZE;
    }

    static constexpr uint32_t GetFrameQueueSize() { return FRAME_QUEUE_SIZE; }
    inline uint32_t GetCurrentFrameIdx() const { return m_currentFrameIdx; }
    vk::Fence& GetCurrentFrameRenderingFence() { return m_frameData[m_currentFrameIdx].m_currentlyRendering; }

    // -- Query properties
    SwapchainSupportDetails GetSwapchainSupport();
    static bool CheckDeviceExtensionsSupport(const vk::PhysicalDevice& dev, Vector<const char*> requiredExtensions);
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format FindDepthFormat();
    vk::Format FindSupportedFormat(const Vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    bool SupportsBlittingToLinearImages();
    inline vk::SampleCountFlagBits GetMSAASamples() const { return m_msaaSamples; }
    inline vk::FormatProperties GetFormatProperties(vk::Format format) { return m_physical.getFormatProperties(format); }
    inline const vk::PhysicalDeviceProperties GetPhysicalDeviceProperties() const { return m_physical.getProperties(); }

    // -- Helpers
    /// @brief Pad a given size to meet the pRenderEngine's alignment requirements.
    /// @return: Adjusted size
    /// @note https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
    size_t PadUniformBufferSize(size_t originalSize);

    // -- Debug
    /// @brief Add a name to a vulkan object for debugging purposes.
    /// @param object: vulkan object to add a debug name to
    /// @param name: debug name
    template <class T>
    void SetDebugUtilsObjectName(T object, std::string name)
    {
        vk::DebugUtilsObjectNameInfoEXT debugName = {
            .objectType = object.objectType,
            .objectHandle = (uint64_t) (typename T::CType) object,
            .pObjectName = name.c_str(),
        };

        m_logical.setDebugUtilsObjectNameEXT(debugName);
    }

    // -- Queues
    Queue& GetGraphicsQueue() { return m_graphicsQueue; }
    Queue& GetPresentQueue() { return m_presentQueue; }
    Queue& GetTransferQueue() { return m_transferQueue; }

    // -- Command Pools
    inline TransientCommandPool& GetTransferTransientCommandPool(uint32_t threadIdx = 0) { return m_frameData[m_currentFrameIdx].m_threadData[threadIdx].m_transferTransientCommandPool; };
    inline TransientCommandPool& GetGraphicsTransientCommandPool(uint32_t threadIdx = 0) { return m_frameData[m_currentFrameIdx].m_threadData[threadIdx].m_graphicsTransientCommandPool; };
    inline TransferQueuePersistentCommandPool& GetTransferPersistentCommandPool(uint32_t threadIdx = 0) { return m_frameData[m_currentFrameIdx].m_threadData[threadIdx].m_transferPersistentCommandPool; };
    inline GraphicsQueuePersistentCommandPool& GetGraphicsPersistentCommandPool(uint32_t threadIdx = 0) { return m_frameData[m_currentFrameIdx].m_threadData[threadIdx].m_graphicsPersistentCommandPool; };

    // -- Descriptors
    inline vk::DescriptorPool& GetDescriptorPool() { return m_descriptorAllocator.GetActivePool(); }

    /// @brief Allocates a single descriptor set from this pRenderEngine's default pool.
    /// @todo Handle multi-threading.
    template <typename T>
    vk::DescriptorSet AllocateDescriptorSet() { return std::move(m_descriptorAllocator.Allocate(&GetDescriptorSetLayout<T>())); }

    vk::DescriptorSet AllocateDescriptorSet(const vk::DescriptorSetLayout* pDescriptorSetLayout) { return std::move(m_descriptorAllocator.Allocate(pDescriptorSetLayout)); }

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

            vk::DescriptorSetLayoutCreateInfo info = {
                .bindingCount = static_cast<uint32_t>(bindings.size()),
                .pBindings = bindings.data(),
            };

            auto [result, layout] = m_logical.createDescriptorSetLayout(info);
            SetDebugUtilsObjectName(layout, typeid(T).name());

            iter = m_descriptorSetLayoutsCache.emplace(type_index, layout).first;
        }
        return iter->second;
    }
};
} // namespace aln