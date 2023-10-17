#include "device.hpp"
#include "commandpool.hpp"
#include "instance.hpp"
#include <assert.h>
#include <set>

#include <vulkan/vulkan.hpp>

namespace aln::vkg
{

/// @brief Create a vulkan device.
void Device::Initialize(vkg::Instance* pInstance, const vk::SurfaceKHR& surface)
{
    m_pInstance = pInstance;
    m_physical = PickPhysicalDevice(surface);
    m_gpuProperties = m_physical.getProperties();

    CreateLogicalDevice(surface);

    auto threadCount = std::thread::hardware_concurrency();
    for (auto threadIdx = 0; threadIdx <= threadCount; ++threadIdx)
    {
        auto& threadData = m_threadData.emplace_back();
        threadData.m_graphicsCommandPool.Initialize(&m_logical.get(), &m_queues.graphics, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        threadData.m_transferCommandPool.Initialize(&m_logical.get(), &m_queues.transfer, vk::CommandPoolCreateFlagBits::eTransient);

        SetDebugUtilsObjectName(threadData.m_graphicsCommandPool.m_vkCommandPool.get(), "Graphics Command Pool (Thread " + threadIdx + ')');
        SetDebugUtilsObjectName(threadData.m_transferCommandPool.m_vkCommandPool.get(), "Transfer Command Pool (Thread " + threadIdx + ')');
    }

    m_msaaSamples = GetMaxUsableSampleCount();
    m_descriptorAllocator.Init(&m_logical.get());
}

void Device::Shutdown()
{
    // TODO: Explicitely destry vulkan resources once the API is better !
    /* for (auto& threadData : m_threadData)
     {
         threadData.m_graphicsCommandPool.Shutdown();
         threadData.m_transferCommandPool.Shutdown();
     }

     m_logical.reset();*/
}

SwapchainSupportDetails Device::GetSwapchainSupport(const vk::SurfaceKHR& surface)
{
    // TODO: Store support as class attribute ?
    return SwapchainSupportDetails(m_physical, surface);
}

bool Device::CheckDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice, Vector<const char*> requiredExtensions)
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

uint32_t Device::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
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

vk::Format Device::FindDepthFormat()
{
    // TODO: Cache return value
    return FindSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format Device::FindSupportedFormat(const Vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
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

size_t Device::PadUniformBufferSize(size_t originalSize)
{
    // Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = m_gpuProperties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0)
    {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

bool Device::SupportsBlittingToLinearImages()
{
    auto formatProps = GetFormatProperties(vk::Format::eR8G8B8A8Unorm);
    if (!(formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst))
    {
        return false;
    }
    return true;
}

void Device::CreateLogicalDevice(const vk::SurfaceKHR& surface)
{
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

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features> chain = {
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
    };

    vk::DeviceCreateInfo deviceCreateInfo = {
        .pNext = &chain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(m_extensions.size()),
        .ppEnabledExtensionNames = m_extensions.data(),
    };

    Vector<const char*> validationLayers;
    if (m_pInstance->ValidationLayersEnabled())
    {
        validationLayers = m_pInstance->GetValidationLayers();
        deviceCreateInfo.enabledLayerCount = static_cast<uint_fast32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    // Create the device
    m_logical = m_physical.createDeviceUnique(deviceCreateInfo).value;

    // Create queues
    m_queues.graphics = Queue(m_logical.get(), queueFamilyIndices.graphicsFamily.value());
    m_queues.present = Queue(m_logical.get(), queueFamilyIndices.presentFamily.value());
    m_queues.transfer = Queue(m_logical.get(), queueFamilyIndices.transferFamily.value());

    SetDebugUtilsObjectName(m_queues.transfer.GetVkQueue(), "Transfer Queue");
    if (queueFamilyIndices.graphicsFamily == queueFamilyIndices.presentFamily)
    {
        SetDebugUtilsObjectName(m_queues.present.GetVkQueue(), "Graphics/Present Queue");
    }
    else
    {
        SetDebugUtilsObjectName(m_queues.present.GetVkQueue(), "Present Queue");
        SetDebugUtilsObjectName(m_queues.graphics.GetVkQueue(), "Graphics Queue");
    }
}

/// @brief Select a suitable GPU among the detected ones.
vk::PhysicalDevice Device::PickPhysicalDevice(const vk::SurfaceKHR& surface)
{
    // TODO: core::Instance could wrap this call and keep a list of devices cached... but it's not necessary right now
    auto devices = m_pInstance->GetVkInstance().enumeratePhysicalDevices().value;

    if (devices.empty())
    {
        throw std::runtime_error("Failed to find GPUs w/ Vulkan support.");
    }

    for (const auto d : devices)
    {
        if (Device::IsDeviceSuitable(d, surface, m_extensions))
        {
            return d;
        }
    }

    throw std::runtime_error("Failed to find a suitable GPU.");
}

vk::SampleCountFlagBits Device::GetMaxUsableSampleCount()
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

/// @brief Check if a device is suitable for the specified surface and requested extensions.
bool Device::IsDeviceSuitable(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface, Vector<const char*> requiredExtensions)
{
    auto familyIndices = Queue::FamilyIndices(device, surface);
    bool extensionsSupported = CheckDeviceExtensionsSupport(device, requiredExtensions);

    bool swapchainAdequate = false;
    if (extensionsSupported)
    {
        auto swapChainSupport = SwapchainSupportDetails(device, surface);
        // In this tutorial a device is adequate as long as it supports at least one image format and one supported presentation mode.
        swapchainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // TODO : Instead of enforcing features, we can disable their usage if not available
    auto supportedFeatures = device.getFeatures();
    return familyIndices.IsComplete() && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
}
}; // namespace aln::vkg