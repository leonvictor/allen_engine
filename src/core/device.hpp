#pragma once

#include <vulkan/vulkan.hpp>
#include <set>
#include <assert.h>

namespace core {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;
    
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
    }
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};


struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class Device  {
public:
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;

    vk::Instance instance;
    
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceFeatures features;
    core::QueueFamilyIndices queueFamilyIndices;
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties;
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    
    std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    }; // TODO: This is == to the reqExtensions parameters everywhere
    
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    vk::Queue graphicsQueue, presentQueue, transferQueue;

    Device() {};

    Device(const vk::Instance& instance, const vk::SurfaceKHR& surface) {
        this->instance = instance;
        // Pick a suitable device
        physicalDevice = pickPhysicalDevice(instance, surface);
        initProperties();
        // Create logical device
        createLogicalDevice(surface);
        msaaSamples = getMaxUsableSampleCount();
    }

    void destroy() {
        // TODO
    }

    SwapchainSupportDetails getSwapchainSupport(const vk::SurfaceKHR &surface) {
        /* TODO: Store support as class attribute ? */
        return querySwapchainSupport(physicalDevice, surface);
    }

    QueueFamilyIndices getQueueFamilyIndices() {
        return this->queueFamilyIndices;
    }
    
    /** @brief Typecast to vk::Device */
    operator vk::Device() {return logicalDevice; };

    static bool checkDeviceExtensionsSupport(const vk::PhysicalDevice &dev, std::vector<const char*> requiredExtensions) {
        // Populate available extensions list
        std::vector<vk::ExtensionProperties> availableExtensions = dev.enumerateDeviceExtensionProperties();

        // Compare against required ones
        std::set<std::string> requiredExtentionsSet(requiredExtensions.begin(), requiredExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtentionsSet.erase(extension.extensionName);
        }

        return requiredExtentionsSet.empty();
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        // auto memProperties = physicalDevice.getMemoryProperties();
        
        // TODO sometimes, consider heaps...
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if (typeFilter & (1 << i) && // Check if the memory type's bit is set to 1
                (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) { // We also need to be able to write our vertex data to the memory
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type.");
    }

    vk::Format findDepthFormat() {
        return findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    }

    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
        for (vk::Format format : candidates) {
            vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(format);
            if (tiling == vk::ImageTiling::eLinear && (formatProperties.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == vk::ImageTiling::eOptimal && (formatProperties.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("Failed to find a supported format.");
    }

    /* Helper function to add a name to a vulkan object for debugging purposes. 
     * TODO: This should be in Context, probably ?
     * TODO: We should be able to simply pass a C++ wrapper and infer the type and handle.
     * Hints: I tried with a template function but failed to cast to the CType inside each class
     */
    [[deprecated("Replaced with a simpler version, use setDebugUtilsObjectName(object, name) instead.")]]
    void setDebugUtilsObjectName(vk::ObjectType type, uint64_t handle, std::string name) {
            throw
            vk::DebugUtilsObjectNameInfoEXT debugName{ type, handle, name.c_str() };
            logicalDevice.setDebugUtilsObjectNameEXT( debugName, vk::DispatchLoaderDynamic{ instance , vkGetInstanceProcAddr});
        }

    template<class T>
    void setDebugUtilsObjectName(T object, std::string name) {
        vk::DebugUtilsObjectNameInfoEXT debugName { object.objectType, (uint64_t) (typename T::CType) object, name.c_str()};
        logicalDevice.setDebugUtilsObjectNameEXT( debugName, vk::DispatchLoaderDynamic{ instance , vkGetInstanceProcAddr});
    }

private:

    void createLogicalDevice(const vk::SurfaceKHR& surface, const bool enableValidationLayers = true) {
        queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        // Added transfer as a separate queue
        std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value(), queueFamilyIndices.transferFamily.value()};
        
        float queuePriority = 1.0f; // We can assign a priority (float [0,1]) to queue families. Needed even if we have only one
        
        for (uint32_t queueFamily : uniqueQueueFamilies) {
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

        if (enableValidationLayers) {
            deviceCreateInfo.enabledLayerCount = static_cast<uint_fast32_t>(core::validationLayers.size());
            deviceCreateInfo.ppEnabledLayerNames = core::validationLayers.data();
        } else {
            deviceCreateInfo.enabledLayerCount = 0;
        }

        logicalDevice = physicalDevice.createDevice(deviceCreateInfo);
        
        graphicsQueue = logicalDevice.getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
        presentQueue = logicalDevice.getQueue(queueFamilyIndices.presentFamily.value(), 0);
        transferQueue = logicalDevice.getQueue(queueFamilyIndices.transferFamily.value(), 0);
    }

    void initProperties() {
        properties = physicalDevice.getProperties();
        memoryProperties = physicalDevice.getMemoryProperties();
        queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    }

    vk::PhysicalDevice pickPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR &surface) {
        std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

        if (devices.size() == 0) {
            throw std::runtime_error("Failed to find GPUs w/ Vulkan support.");
        }

        for (const auto d : devices) {
            if (isDeviceSuitable(d, surface, extensions)) {
                return d;
            }
        }

        throw std::runtime_error("Failed to find a suitable GPU.");
    }
    
    static SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice& device,  const vk::SurfaceKHR &surface){
        SwapchainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);
        return details;
    };

    vk::SampleCountFlagBits getMaxUsableSampleCount() {
        // TODO: Beurk
        // assert(properties && "Properties were not set. GetMaxUsableSampleCount must be called after.");
        // assert(properties);
        vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
        if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
        if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
        if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
        if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
        if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
        if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }
        return vk::SampleCountFlagBits::e1;
    }

    /* TODO: This is static but only used internally. We may aswell remove the static part and assert that physical device has been picked. */
    static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, const vk::SurfaceKHR& surface) {
        QueueFamilyIndices indices;
        // Assign index to queue families that could be found
        uint32_t queueFamilyCount = 0;
        std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            } else if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
                indices.transferFamily = i;
            }

            if (device.getSurfaceSupportKHR(i, surface)) {
                indices.presentFamily = i;
            }
            //TODO : It's very likely that the queue family that has the "present" capability is the same as
            // the one that has "graphics". In the tutorial they are treated as if they were separate for a uniform approach.
            // We can add logic to explicitly prefer physical devices that support both drawing and presentation in the same queue
            // for improved performance.        

            if (indices.isComplete()) {
                break;
            }
            i++;
        }
        return indices;
    }

    static bool isDeviceSuitable(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface, std::vector<const char*> requiredExtensions) {
        core::QueueFamilyIndices familyIndices = findQueueFamilies(device, surface);
        bool extensionsSupported = checkDeviceExtensionsSupport(device, requiredExtensions);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            core::SwapchainSupportDetails swapChainSupport = querySwapchainSupport(device, surface);
            // In this tutorial a device is adequate as long as it supports at least one image format and one supported presentation mode.
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // TODO : Instead of enforcing features, we can disable their usage if not available
        vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();
        return familyIndices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }
};
}