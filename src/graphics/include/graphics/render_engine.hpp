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

struct SwapchainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    Vector<vk::SurfaceFormatKHR> formats;
    Vector<vk::PresentModeKHR> presentModes;

    SwapchainSupportDetails(const vk::PhysicalDevice& pRenderEngine, const vk::SurfaceKHR& surface)
    {
        capabilities = pRenderEngine.getSurfaceCapabilitiesKHR(surface).value;

        uint32_t formatsCount;
        pRenderEngine.getSurfaceFormatsKHR(surface, &formatsCount, nullptr);
        formats.resize(formatsCount);
        pRenderEngine.getSurfaceFormatsKHR(surface, &formatsCount, formats.data());

        uint32_t presentModesCount;
        pRenderEngine.getSurfacePresentModesKHR(surface, &presentModesCount, nullptr);
        presentModes.resize(presentModesCount);
        pRenderEngine.getSurfacePresentModesKHR(surface, &presentModesCount, presentModes.data());
    };
};

/// @brief Core rendering engine
/// @todo Use "unique" vk structs only when beneficial to avoid unecessary overhead
class RenderEngine
{
    static constexpr uint32_t FRAME_QUEUE_SIZE = 2;

    // Per-frame per-thread data
    struct ThreadData
    {
        CommandPool m_graphicsCommandPool;
        CommandPool m_transferCommandPool;
    };

    // Per-frame in queue data
    struct FrameData
    {
        Vector<ThreadData> m_threadData;

        vk::UniqueSemaphore m_imageAvailableSemaphore; // Signaled when the image is available
        vk::UniqueSemaphore m_renderFinished;          //
        vk::UniqueFence m_inFlight;
    };

  private:
    // Vulkan objects
    Instance m_instance;
    vk::SurfaceKHR m_surface;
    vk::PhysicalDevice m_physical; // Physical pRenderEngine we're associated to.
    vk::UniqueDevice m_logical;    // Wrapped logical vulkan pRenderEngine.
    Swapchain m_swapchain;
    Array<FrameData, FRAME_QUEUE_SIZE> m_frameData;

    Queue m_graphicsQueue;
    Queue m_presentQueue;
    Queue m_transferQueue;

    // Properties and capabilities
    vk::PhysicalDeviceMemoryProperties m_memoryProperties;
    vk::PhysicalDeviceProperties m_gpuProperties;
    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;
    Vector<const char*> m_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Descriptors
    HashMap<std::type_index, vk::UniqueDescriptorSetLayout, std::hash<std::type_index>> m_descriptorSetLayoutsCache;
    DescriptorAllocator m_descriptorAllocator;

    // Runtime
    uint32_t m_currentFrameIdx = 0;

  private:
    void CreateLogicalDevice(const vk::SurfaceKHR& surface);
    vk::PhysicalDevice PickPhysicalDevice(const vk::SurfaceKHR& surface);
    vk::SampleCountFlagBits GetMaxUsableSampleCount();
    static bool IsDeviceSuitable(const vk::PhysicalDevice& pRenderEngine, const vk::SurfaceKHR& surface, Vector<const char*> requiredExtensions);

  public:
    // -- Getters
    // TODO: Those are necessary for now because of some functionnality gravitating outside. Remove when possible !
    inline Instance* GetInstance() { return &m_instance; }
    inline vk::Device& GetVkDevice() { return m_logical.get(); }
    inline vk::PhysicalDevice& GetVkPhysicalDevice() { return m_physical; }
    inline Swapchain& GetSwapchain() { return m_swapchain; }

    // -- Lifetime
    void Initialize(GLFWwindow* pGlfwWindow);
    void Shutdown();
    void WaitIdle() { m_logical->waitIdle(); }

    void StartFrame()
    {
        // Reset all command pools
        /*for (auto& threadData : m_frameData[m_currentFrameIdx].m_threadData)
        {
            threadData.m_graphicsCommandPool.Reset();
            threadData.m_transferCommandPool.Reset();
        }*/
    }

    void EndFrame()
    {
        m_currentFrameIdx = (m_currentFrameIdx + 1) % FRAME_QUEUE_SIZE;
    }

    constexpr uint32_t GetFrameQueueSize() const { return FRAME_QUEUE_SIZE; }
    inline uint32_t GetCurrentFrameIdx() const { return m_currentFrameIdx; }

    // -- Query properties
    SwapchainSupportDetails GetSwapchainSupport(const vk::SurfaceKHR& surface);
    static bool CheckDeviceExtensionsSupport(const vk::PhysicalDevice& dev, Vector<const char*> requiredExtensions);
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format FindDepthFormat();
    vk::Format FindSupportedFormat(const Vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    bool SupportsBlittingToLinearImages();
    inline vk::SampleCountFlagBits GetMSAASamples() const { return m_msaaSamples; }
    inline vk::FormatProperties GetFormatProperties(vk::Format format) { return m_physical.getFormatProperties(format); }
    inline const vk::PhysicalDeviceProperties& GetPhysicalDeviceProperties() const { return m_physical.getProperties(); }

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

        m_logical->setDebugUtilsObjectNameEXT(debugName, m_instance.GetDispatchLoaderDynamic());
    }

    // -- Queues
    inline Queue& GetGraphicsQueue() { return m_graphicsQueue; }
    inline Queue& GetPresentQueue() { return m_presentQueue; }
    inline Queue& GetTransferQueue() { return m_transferQueue; }

    // -- Command Pools
    inline CommandPool& GetTransferCommandPool(uint32_t threadIdx = 0) { return m_frameData[m_currentFrameIdx].m_threadData[threadIdx].m_transferCommandPool; };
    inline CommandPool& GetGraphicsCommandPool(uint32_t threadIdx = 0) { return m_frameData[m_currentFrameIdx].m_threadData[threadIdx].m_graphicsCommandPool; };

    // -- Descriptors
    inline vk::DescriptorPool& GetDescriptorPool() { return m_descriptorAllocator.GetActivePool(); }

    /// @brief Allocates a single descriptor set from this pRenderEngine's default pool.
    /// @todo Handle multi-threading.
    template <typename T>
    vk::UniqueDescriptorSet AllocateDescriptorSet() { return std::move(m_descriptorAllocator.Allocate(&GetDescriptorSetLayout<T>())); }

    vk::UniqueDescriptorSet AllocateDescriptorSet(const vk::DescriptorSetLayout* pDescriptorSetLayout) { return std::move(m_descriptorAllocator.Allocate(pDescriptorSetLayout)); }

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

            auto [result, layout] = m_logical->createDescriptorSetLayoutUnique(info);
            SetDebugUtilsObjectName(layout.get(), typeid(T).name());

            iter = m_descriptorSetLayoutsCache.emplace(type_index, std::move(layout)).first;
        }
        return iter->second.get();
    }
};
} // namespace aln