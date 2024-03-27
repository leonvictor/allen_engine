#include "render_engine.hpp"
#include "commandpool.hpp"
#include "instance.hpp"
#include "window.hpp"

#include <aln_graphics_export.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <assert.h>
#include <set>
#include <thread>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace aln
{

/// @brief Create a vulkan pRenderEngine.
void RenderEngine::Initialize(IWindow* pGlfwWindow)
{
    auto& test = VULKAN_HPP_DEFAULT_DISPATCHER;

    PFN_vkGetInstanceProcAddr getInstanceProcAddr = m_dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(getInstanceProcAddr);

    m_pWindow = pGlfwWindow;

    uint32_t glfwExtensionCount;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    Vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    m_instance.Initialize(extensions);

    m_pWindow->CreateSurface(m_instance.GetVkInstance());
    m_physical = PickPhysicalDevice();
    m_gpuProperties = m_physical.getProperties();

    CreateLogicalDevice();

    m_msaaSamples = GetMaxUsableSampleCount();
    m_descriptorAllocator.Initialize(&m_logical);

    m_pWindow->CreateSwapchain(this);

    auto threadCount = std::thread::hardware_concurrency();
    for (auto frameIdx = 0; frameIdx < FRAME_QUEUE_SIZE; ++frameIdx)
    {
        auto& frameData = m_frameData[frameIdx];
        for (auto threadIdx = 0; threadIdx <= threadCount; ++threadIdx)
        {
            auto& threadData = frameData.m_threadData.emplace_back();
            threadData.m_graphicsTransientCommandPool.Initialize(&m_logical, &m_graphicsQueue);
            threadData.m_transferTransientCommandPool.Initialize(&m_logical, &m_transferQueue);
            threadData.m_graphicsPersistentCommandPool.Initialize(&m_logical, &m_graphicsQueue);
            threadData.m_transferPersistentCommandPool.Initialize(&m_logical, &m_transferQueue);

            SetDebugUtilsObjectName(threadData.m_graphicsTransientCommandPool.m_commandPool, "Graphics Transient Command Pool (Image " + std::to_string(frameIdx) + ", Thread " + std::to_string(threadIdx) + ")");
            SetDebugUtilsObjectName(threadData.m_transferTransientCommandPool.m_commandPool, "Transfer Transient Command Pool (Image " + std::to_string(frameIdx) + ", Thread " + std::to_string(threadIdx) + ")");
            SetDebugUtilsObjectName(threadData.m_graphicsPersistentCommandPool.m_commandPool, "Graphics Persistent Command Pool (Image " + std::to_string(frameIdx) + ", Thread " + std::to_string(threadIdx) + ")");
            SetDebugUtilsObjectName(threadData.m_transferPersistentCommandPool.m_commandPool, "Transfer Persistent Command Pool (Image " + std::to_string(frameIdx) + ", Thread " + std::to_string(threadIdx) + ")");
        }

        vk::FenceCreateInfo fenceCreateInfo = {
            .flags = vk::FenceCreateFlagBits::eSignaled,
        };

        frameData.m_currentlyRendering = m_logical.createFence(fenceCreateInfo).value;
    }
}

void RenderEngine::Shutdown()
{
    for (auto& frameData : m_frameData)
    {
        m_logical.destroyFence(frameData.m_currentlyRendering);

        for (auto& threadData : frameData.m_threadData)
        {
            threadData.m_graphicsTransientCommandPool.Shutdown();
            threadData.m_transferTransientCommandPool.Shutdown();
            threadData.m_graphicsPersistentCommandPool.Shutdown();
            threadData.m_transferPersistentCommandPool.Shutdown();
        }
    }

    m_pWindow->DestroySwapchain(this);

    for (auto& [typeIndex, descriptorSetLayout] : m_descriptorSetLayoutsCache)
    {
        m_logical.destroyDescriptorSetLayout(descriptorSetLayout);
    }
    m_descriptorSetLayoutsCache.clear();

    m_descriptorAllocator.Shutdown();
    m_logical.destroy();

    m_pWindow->DestroySurface(m_instance.GetVkInstance());

    m_instance.Shutdown();
}

SwapchainSupportDetails RenderEngine::GetSwapchainSupport()
{
    // TODO: Store support as class attribute ?
    auto& surface = m_pWindow->GetSurface();
    return SwapchainSupportDetails(m_physical, surface);
}

bool RenderEngine::CheckDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice, Vector<const char*> requiredExtensions)
{
    // Populate available extensions list
    auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties().value;

    // Compare against required ones
    std::set<std::string> requiredExtentionsSet(requiredExtensions.begin(), requiredExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtentionsSet.erase(extension.extensionName);
    }

    return requiredExtentionsSet.empty();
}

uint32_t RenderEngine::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    auto memProperties = m_physical.getMemoryProperties();

    // TODO sometimes, consider heaps...
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && // Check if the memory type's bit is set to 1
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        { // We also need to be able to write our vertex data to the memory
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type.");
}

vk::Format RenderEngine::FindDepthFormat()
{
    // TODO: Cache return value
    return FindSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format RenderEngine::FindSupportedFormat(const Vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates)
    {
        auto formatProperties = GetFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (formatProperties.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (formatProperties.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    throw std::runtime_error("Failed to find a supported format.");
}

size_t RenderEngine::PadUniformBufferSize(size_t originalSize)
{
    // Calculate required alignment based on minimum pRenderEngine offset alignment
    size_t minUboAlignment = m_gpuProperties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0)
    {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

bool RenderEngine::SupportsBlittingToLinearImages()
{
    auto formatProps = GetFormatProperties(vk::Format::eR8G8B8A8Unorm);
    if (!(formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst))
    {
        return false;
    }
    return true;
}

void RenderEngine::CreateLogicalDevice()
{
    assert(m_pWindow != nullptr && m_physical);
    auto& surface = m_pWindow->GetSurface();

    auto queueFamilyIndices = Queue::FamilyIndices(m_physical, surface);

    Vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value(),
        queueFamilyIndices.transferFamily.value(),
    };

    float queuePriority = 1.0f; // We can assign a priority (float [0,1]) to queue families. Needed even if we have only one

    for (auto queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo = {
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features>
        chain = {
            {
                // 1.0 features
                .features = {
                    .sampleRateShading = vk::True,
                    .samplerAnisotropy = vk::True,
                    .fragmentStoresAndAtomics = vk::True,
                },
            },
            // 1.1 features
            {},
            // 1.2 features
            {
                .timelineSemaphore = vk::True,
            },
            // 1.3 features
            {
                .synchronization2 = vk::True,
            },
        };

    vk::DeviceCreateInfo deviceCreateInfo = {
        .pNext = &chain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(m_extensions.size()),
        .ppEnabledExtensionNames = m_extensions.data(),
    };

    Vector<const char*> validationLayers;
    if (m_instance.ValidationLayersEnabled())
    {
        validationLayers = m_instance.GetValidationLayers();
        deviceCreateInfo.enabledLayerCount = static_cast<uint_fast32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    // Create the pRenderEngine
    m_logical = m_physical.createDevice(deviceCreateInfo).value;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_logical);

    // Create queues
    m_graphicsQueue = Queue(m_logical, queueFamilyIndices.graphicsFamily.value());
    m_presentQueue = Queue(m_logical, queueFamilyIndices.presentFamily.value());
    m_transferQueue = Queue(m_logical, queueFamilyIndices.transferFamily.value());

    SetDebugUtilsObjectName(m_transferQueue.GetVkQueue(), "Transfer Queue");
    if (queueFamilyIndices.graphicsFamily == queueFamilyIndices.presentFamily)
    {
        SetDebugUtilsObjectName(m_presentQueue.GetVkQueue(), "Graphics/Present Queue");
    }
    else
    {
        SetDebugUtilsObjectName(m_presentQueue.GetVkQueue(), "Present Queue");
        SetDebugUtilsObjectName(m_graphicsQueue.GetVkQueue(), "Graphics Queue");
    }
}

/// @brief Select a suitable GPU among the detected ones.
vk::PhysicalDevice RenderEngine::PickPhysicalDevice()
{
    assert(m_pWindow != nullptr);
    const auto& surface = m_pWindow->GetSurface();

    // TODO: core::Instance could wrap this call and keep a list of devices cached... but it's not necessary right now
    auto devices = m_instance.GetVkInstance().enumeratePhysicalDevices().value;

    if (devices.empty())
    {
        throw std::runtime_error("Failed to find GPUs w/ Vulkan support.");
    }

    for (const auto d : devices)
    {
        if (RenderEngine::IsDeviceSuitable(d, surface, m_extensions))
        {
            return d;
        }
    }

    throw std::runtime_error("Failed to find a suitable GPU.");
}

vk::SampleCountFlagBits RenderEngine::GetMaxUsableSampleCount()
{
    vk::SampleCountFlags counts = m_gpuProperties.limits.framebufferColorSampleCounts & m_gpuProperties.limits.framebufferDepthSampleCounts;

    // TODO: Beurk
    if (counts & vk::SampleCountFlagBits::e64)
    {
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e32)
    {
        return vk::SampleCountFlagBits::e32;
    }
    if (counts & vk::SampleCountFlagBits::e16)
    {
        return vk::SampleCountFlagBits::e16;
    }
    if (counts & vk::SampleCountFlagBits::e8)
    {
        return vk::SampleCountFlagBits::e8;
    }
    if (counts & vk::SampleCountFlagBits::e4)
    {
        return vk::SampleCountFlagBits::e4;
    }
    if (counts & vk::SampleCountFlagBits::e2)
    {
        return vk::SampleCountFlagBits::e2;
    }
    return vk::SampleCountFlagBits::e1;
}

/// @brief Check if a physical device is suitable for the specified surface and requested extensions.
bool RenderEngine::IsDeviceSuitable(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, Vector<const char*> requiredExtensions)
{
    assert(physicalDevice && surface);

    auto familyIndices = Queue::FamilyIndices(physicalDevice, surface);
    bool extensionsSupported = CheckDeviceExtensionsSupport(physicalDevice, requiredExtensions);

    bool swapchainAdequate = false;
    if (extensionsSupported)
    {
        auto swapChainSupport = SwapchainSupportDetails(physicalDevice, surface);
        // In this tutorial a pRenderEngine is adequate as long as it supports at least one image format and one supported presentation mode.
        swapchainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // TODO : Instead of enforcing features, we can disable their usage if not available
    auto supportedFeatures = physicalDevice.getFeatures();
    return familyIndices.IsComplete() && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
}
}; // namespace aln