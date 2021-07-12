#include "device.hpp"
#include "commandpool.hpp"
#include "instance.hpp"
#include <assert.h>
#include <set>

#include <vulkan/vulkan.hpp>

namespace vkg
{

Device::Device() {}

/// @brief Create a vulkan device.
Device::Device(const vk::SurfaceKHR& surface)
{
    m_physical = PickPhysicalDevice(surface);

    CreateLogicalDevice(surface);

    m_commandpools.graphics = CommandPool(&m_logical.get(), &m_queues.graphics, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    m_commandpools.transfer = CommandPool(&m_logical.get(), &m_queues.transfer, vk::CommandPoolCreateFlagBits::eTransient);
    m_msaaSamples = GetMaxUsableSampleCount();

    // TODO: Refactor
    int MAX_OBJECTS = 10000;

    // Create descriptor pools.
    /// TODO: Find a principled way of deciding the max number of elements
    std::array<vk::DescriptorPoolSize, 3> poolSizes;
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = (MAX_OBJECTS * 3) + 1; // +1 for skybox. TODO: This should be dynamic
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = MAX_OBJECTS + 1; // +1 TODO Same here
    poolSizes[2].type = vk::DescriptorType::eStorageBuffer;
    poolSizes[2].descriptorCount = 1; // For now, only used by lights

    vk::DescriptorPoolCreateInfo createInfo;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = (MAX_OBJECTS * 2) + 2;                              // TODO: +2 is for lights / skybox. Make it less hardcoded.  * 2 for color picker
    createInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet; // Necessary for automatic freeing
    m_descriptorPool = m_logical->createDescriptorPoolUnique(createInfo);
}

SwapchainSupportDetails Device::GetSwapchainSupport(const vk::SurfaceKHR& surface)
{
    // TODO: Store support as class attribute ?
    return SwapchainSupportDetails(m_physical, surface);
}

bool Device::CheckDeviceExtensionsSupport(const vk::PhysicalDevice& physicalDevice, std::vector<const char*> requiredExtensions)
{
    // Populate available extensions list
    auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

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

vk::Format Device::FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
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

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value(),
        queueFamilyIndices.transferFamily.value(),
    };

    float queuePriority = 1.0f; // We can assign a priority (float [0,1]) to queue families. Needed even if we have only one

    for (auto queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo{
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_extensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = m_extensions.data();

    vk::PhysicalDeviceFeatures features;
    features.sampleRateShading = VK_TRUE;
    features.samplerAnisotropy = VK_TRUE;
    features.fragmentStoresAndAtomics = VK_TRUE;

    deviceCreateInfo.pEnabledFeatures = &features;

    std::vector<const char*> validationLayers;
    if (Instance::ValidationLayersEnabled())
    {
        validationLayers = Instance::GetValidationLayers();
        deviceCreateInfo.enabledLayerCount = static_cast<uint_fast32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    // Create the device
    m_logical = m_physical.createDeviceUnique(deviceCreateInfo);

    // Create queues
    m_queues.graphics = Queue(m_logical.get(), queueFamilyIndices.graphicsFamily.value());
    m_queues.present = Queue(m_logical.get(), queueFamilyIndices.presentFamily.value());
    m_queues.transfer = Queue(m_logical.get(), queueFamilyIndices.transferFamily.value());
}

/// @brief Select a suitable GPU among the detected ones.
vk::PhysicalDevice Device::PickPhysicalDevice(const vk::SurfaceKHR& surface)
{
    // TODO: core::Instance could wrap this call and keep a list of devices cached... but it's not necessary right now
    std::vector<vk::PhysicalDevice> devices = Instance::Get().enumeratePhysicalDevices();

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
    auto properties = m_physical.getProperties();
    vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

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
bool Device::IsDeviceSuitable(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface, std::vector<const char*> requiredExtensions)
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
}; // namespace vkg