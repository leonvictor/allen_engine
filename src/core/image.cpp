#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>

#include "device.hpp"

namespace core {
    class Image {
    public:
        vk::Image image;
        vk::ImageView view;
        vk::DeviceMemory memory;
        
        /* Empty ctor to avoid errors. We should be able to get rid of it later on*/
        Image() {}

        Image(core::Device &device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
                vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask) {

            initImage(device, width, height, mipLevels, numSamples, format, tiling, usage);
            initMemory(device, memProperties);
            initView(device, format, aspectMask, mipLevels); // TODO: Directly use 
            
        }

        Image(core::Device &device, vk::Image image, vk::MemoryPropertyFlags memProperties) {
            this->image = image; // TODO: shared_ptr ?
            initMemory(device, memProperties);
        }

        // TODO: A destructor may not have parameters, so we need to keep a pointer to device.
        // ~Image(core::VulkanDevice &device) { 
        //     device.logicalDevice.destroyImageView(view);
        //     device.logicalDevice.destroyImage(image);
        //     device.logicalDevice.freeMemory(memory);
        // }

        /* helper function to create image views
        * @note: TODO: Should this be somewere else ? It doesn't depend on image members at all and is called from other places. 
        * If so what would be a good place ? Inside device ?
        */
        static vk::ImageView createImageView(core::Device &device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels) {
            vk::ImageViewCreateInfo createInfo;
            createInfo.format = format;
            createInfo.image = image;
            createInfo.viewType = vk::ImageViewType::e2D;
            createInfo.subresourceRange.aspectMask = aspectMask;
            createInfo.subresourceRange.layerCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.levelCount = mipLevels;
            createInfo.subresourceRange.baseMipLevel = 0;

            return device.logicalDevice.createImageView(createInfo);
        }

    private:
        // std::shared_ptr<core::VulkanDevice> device; // TODO
        void initImage(core::Device &device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
            vk::ImageUsageFlags usage) {

            vk::ImageCreateInfo imageInfo;
            imageInfo.imageType = vk::ImageType::e2D; 
            imageInfo.extent.width = static_cast<uint32_t>(width);
            imageInfo.extent.height = static_cast<uint32_t>(height);
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = vk::ImageLayout::eUndefined; // The very first iteration will discard the texels
            imageInfo.usage = usage;
            imageInfo.sharingMode = vk::SharingMode::eExclusive; // TODO: Ok ? We need to use both transfer and graphics queues
            imageInfo.samples = numSamples;

            image = device.logicalDevice.createImage(imageInfo);
        }

        void initMemory(core::Device &device, vk::MemoryPropertyFlags memProperties) {
            auto memRequirements = device.logicalDevice.getImageMemoryRequirements(image);

            vk::MemoryAllocateInfo allocInfo = {};
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, memProperties);

            memory = device.logicalDevice.allocateMemory(allocInfo);
            device.logicalDevice.bindImageMemory(image, memory, 0);
        }

        void initView(core::Device &device, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels) {
            view = createImageView(device, this->image, format, aspectMask, mipLevels);
        }
    };
}