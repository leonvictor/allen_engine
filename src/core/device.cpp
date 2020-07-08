#include "device.hpp"
#include "commandpool.hpp"
#include <assert.h>
#include <set>
#include <vulkan/vulkan.hpp>

namespace core
{

Device::Device() {}

Device::Device(const vk::UniqueInstance& instance, const vk::UniqueSurfaceKHR& surface)
{
    physical = pickPhysicalDevice(instance, surface);
    initProperties();
    createLogicalDevice(surface);
    createCommandPools();
    msaaSamples = getMaxUsableSampleCount();
}

void Device::createCommandPools()
{
    commandpools.graphics = core::CommandPool(logical.get(), queues.graphics, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    commandpools.transfer = core::CommandPool(logical.get(), queues.transfer, vk::CommandPoolCreateFlagBits::eTransient);
}

SwapchainSupportDetails Device::getSwapchainSupport(const vk::UniqueSurfaceKHR& surface)
{
    /* TODO: Store support as class attribute ? */
    return querySwapchainSupport(physical, surface);
}

QueueFamilyIndices Device::getQueueFamilyIndices()
{
    return this->queueFamilyIndices;
}

bool Device::checkDeviceExtensionsSupport(const vk::PhysicalDevice& dev, std::vector<const char*> requiredExtensions)
{
    // Populate available extensions list
    std::vector<vk::ExtensionProperties> availableExtensions = dev.enumerateDeviceExtensionProperties();

    // Compare against required ones
    std::set<std::string> requiredExtentionsSet(requiredExtensions.begin(), requiredExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtentionsSet.erase((std::string) extension.extensionName);
    }

    return requiredExtentionsSet.empty();
}

uint32_t Device::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    // auto memProperties = physicalDevice.getMemoryProperties();

    // TODO sometimes, consider heaps...
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && // Check if the memory type's bit is set to 1
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        { // We also need to be able to write our vertex data to the memory
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type.");
}

vk::Format Device::findDepthFormat()
{
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format Device::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates)
    {
        vk::FormatProperties formatProperties = physical.getFormatProperties(format);
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

// Check if the device supports blitting to linear images
// TODO: Make more versatile
bool Device::supportsBlittingToLinearImages()
{
    vk::FormatProperties formatProps = physical.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
    if (!(formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst))
    {
        return false;
    }
    return true;
}

void Device::createLogicalDevice(const vk::UniqueSurfaceKHR& surface, const bool enableValidationLayers)
{
    queueFamilyIndices = findQueueFamilies(physical, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    // Added transfer as a separate queue
    std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value(), queueFamilyIndices.transferFamily.value()};

    float queuePriority = 1.0f; // We can assign a priority (float [0,1]) to queue families. Needed even if we have only one

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamily, 1, &queuePriority);
        queueCreateInfos.push_back(queueCreateInfo);
    }

    features.samplerAnisotropy = VK_TRUE;
    features.sampleRateShading = VK_TRUE;
    features.fragmentStoresAndAtomics = VK_TRUE;

    vk::DeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &features;

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint_fast32_t>(core::validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = core::validationLayers.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    logical = physical.createDeviceUnique(deviceCreateInfo);

    queues.graphics = Queue(logical.get(), queueFamilyIndices.graphicsFamily.value());
    queues.present = Queue(logical.get(), queueFamilyIndices.presentFamily.value());
    queues.transfer = Queue(logical.get(), queueFamilyIndices.transferFamily.value());
}

void Device::initProperties()
{
    properties = physical.getProperties();
    memoryProperties = physical.getMemoryProperties();
    queueFamilyProperties = physical.getQueueFamilyProperties();
}

vk::PhysicalDevice Device::pickPhysicalDevice(const vk::UniqueInstance& instance, const vk::UniqueSurfaceKHR& surface)
{
    std::vector<vk::PhysicalDevice> devices = instance.get().enumeratePhysicalDevices();

    if (devices.size() == 0)
    {
        throw std::runtime_error("Failed to find GPUs w/ Vulkan support.");
    }

    for (const auto d : devices)
    {
        if (isDeviceSuitable(d, surface, extensions))
        {
            return d;
        }
    }

    throw std::runtime_error("Failed to find a suitable GPU.");
}

SwapchainSupportDetails Device::querySwapchainSupport(const vk::PhysicalDevice& device, const vk::UniqueSurfaceKHR& surface)
{
    SwapchainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface.get());
    details.formats = device.getSurfaceFormatsKHR(surface.get());
    details.presentModes = device.getSurfacePresentModesKHR(surface.get());
    return details;
};

vk::SampleCountFlagBits Device::getMaxUsableSampleCount()
{
    // TODO: Beurk
    // assert(properties && "Properties were not set. GetMaxUsableSampleCount must be called after.");
    // assert(properties);
    vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
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

/* TODO: This is static but only used internally. We may aswell remove the static part and assert that physical device has been picked. */
QueueFamilyIndices Device::findQueueFamilies(vk::PhysicalDevice device, const vk::UniqueSurfaceKHR& surface)
{
    QueueFamilyIndices indices;
    // Assign index to queue families that could be found
    uint32_t queueFamilyCount = 0;
    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }
        else if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
        {
            indices.transferFamily = i;
        }

        if (device.getSurfaceSupportKHR(i, surface.get()))
        {
            indices.presentFamily = i;
        }
        //TODO : It's very likely that the queue family that has the "present" capability is the same as
        // the one that has "graphics". In the tutorial they are treated as if they were separate for a uniform approach.
        // We can add logic to explicitly prefer physical devices that support both drawing and presentation in the same queue
        // for improved performance.

        if (indices.isComplete())
        {
            break;
        }
        i++;
    }
    return indices;
}

bool Device::isDeviceSuitable(const vk::PhysicalDevice& device, const vk::UniqueSurfaceKHR& surface, std::vector<const char*> requiredExtensions)
{
    core::QueueFamilyIndices familyIndices = findQueueFamilies(device, surface);
    bool extensionsSupported = checkDeviceExtensionsSupport(device, requiredExtensions);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        core::SwapchainSupportDetails swapChainSupport = querySwapchainSupport(device, surface);
        // In this tutorial a device is adequate as long as it supports at least one image format and one supported presentation mode.
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // TODO : Instead of enforcing features, we can disable their usage if not available
    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();
    return familyIndices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}
}; // namespace core