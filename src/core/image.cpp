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
        
        std::shared_ptr<core::Device> device;

        /* Empty ctor to avoid errors. We should be able to get rid of it later on*/
        Image() {}

        Image(std::shared_ptr<core::Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
                vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask) {

            this->device = device;
            initImage(width, height, mipLevels, numSamples, format, tiling, usage);
            initMemory(memProperties);
            initView(format, aspectMask, mipLevels); // TODO: Directly use 
            
        }

        Image(std::shared_ptr<core::Device> device, vk::Image image, vk::MemoryPropertyFlags memProperties) {
            this->device = device;
            this->image = image; // TODO: shared_ptr ?
            initMemory(memProperties);
        }

        ~Image() { 
            // device->logicalDevice.destroyImageView(view);
            // device->logicalDevice.destroyImage(image);
            // device->logicalDevice.freeMemory(memory);
        }

        /* helper function to create image views
        * @note: TODO: Should this be somewere else ? It doesn't depend on image members at all and is called from other places. 
        * If so what would be a good place ? Inside device ?
        */
        static vk::ImageView createImageView(std::shared_ptr<core::Device> device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels) {
            vk::ImageViewCreateInfo createInfo;
            createInfo.format = format;
            createInfo.image = image;
            createInfo.viewType = vk::ImageViewType::e2D;
            createInfo.subresourceRange.aspectMask = aspectMask;
            createInfo.subresourceRange.layerCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.levelCount = mipLevels;
            createInfo.subresourceRange.baseMipLevel = 0;

            return device->logicalDevice.createImageView(createInfo);
        }

    private:
        // std::shared_ptr<core::Device> device; // TODO
        void initImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
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

            image = device->logicalDevice.createImage(imageInfo);
        }

        void initMemory(vk::MemoryPropertyFlags memProperties) {
            auto memRequirements = device->logicalDevice.getImageMemoryRequirements(image);

            vk::MemoryAllocateInfo allocInfo = {};
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, memProperties);

            memory = device->logicalDevice.allocateMemory(allocInfo);
            device->logicalDevice.bindImageMemory(image, memory, 0);
        }

        void initView(vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels) {
            view = createImageView(device, this->image, format, aspectMask, mipLevels);
        }
    };
}