#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>

#include "device.hpp"
#include "context.hpp"
#include "commandpool.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace core
{

// Minimal Cpp wrapper for STB loader
// TODO by order of preference :
// 1) Get rid of it
// 2) Find it a better name
// 3) Move it somewhere else
struct ImageFile
{
    stbi_uc* pixels;
    int width, height, channels;

    ImageFile(std::string path)
    {
        load(path);
    }

    void load(std::string path)
    {
        pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels)
        {
            throw std::runtime_error("Failed to load image at " + path);
        }
    }
};

class Image
{
protected:
    std::shared_ptr<core::Device> device;

    void initImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage, int arrayLayers = 1, vk::ImageCreateFlagBits flags = {})
    {

        vk::ImageCreateInfo imageInfo;
        imageInfo.flags = flags;
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined; // The very first iteration will discard the texels
        imageInfo.usage = usage;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.samples = numSamples;

        image = device->logicalDevice.createImage(imageInfo);
    }

    void initMemory(vk::MemoryPropertyFlags memProperties)
    {
        auto memRequirements = device->logicalDevice.getImageMemoryRequirements(image);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, memProperties);

        memory = device->logicalDevice.allocateMemory(allocInfo);
        device->logicalDevice.bindImageMemory(image, memory, 0);
    }

    void initView(vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels, vk::ImageViewType viewtype = vk::ImageViewType::e2D, int layerCount = 1)
    {
        assert(!view && "Image view is already initialized.");
        view = createImageView(device, this->image, format, aspectMask, mipLevels, viewtype, layerCount);
    }

public:
    vk::Image image;
    vk::ImageView view;
    vk::DeviceMemory memory;

    int mipLevels;

    /* Empty ctor to avoid errors. We should be able to get rid of it later on*/
    Image() {}

    Image(std::shared_ptr<core::Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
          vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask)
    {

        this->device = device;

        initImage(width, height, mipLevels, numSamples, format, tiling, usage);
        initMemory(memProperties);
        initView(format, aspectMask, mipLevels);
    }

    Image(std::shared_ptr<core::Device> device, vk::Image image, vk::MemoryPropertyFlags memProperties)
    {
        this->device = device;
        this->image = image;
        initMemory(memProperties);
    }

    void destroy()
    {
        // TODO: We might not need this with Unique stuff ?
        device->logicalDevice.destroyImageView(view);
        device->logicalDevice.destroyImage(image);
        device->logicalDevice.freeMemory(memory);
    }

    operator vk::Image() { return image; }

    /* Transition  
        * TODO 
        * - Is this the same format ? If so, move it to an attribute 
        * - Is this the same mipLevels ? //
        * 
        */
    void transitionLayout(std::shared_ptr<core::Context> context, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels, int layerCount = 1)
    {
        vk::ImageMemoryBarrier memoryBarrier;
        memoryBarrier.oldLayout = oldLayout;
        memoryBarrier.newLayout = newLayout;
        //TODO: Specify transferQueue here ?
        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.image = image;
        memoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        memoryBarrier.subresourceRange.layerCount = layerCount;
        memoryBarrier.subresourceRange.baseArrayLayer = 0;
        memoryBarrier.subresourceRange.levelCount = mipLevels;
        memoryBarrier.subresourceRange.baseMipLevel = 0;
        memoryBarrier.srcAccessMask = vk::AccessFlags(); // Which operations must happen before the barrier
        memoryBarrier.dstAccessMask = vk::AccessFlags(); // ... and after

        vk::Queue *queue;
        core::CommandPool *commandPool;
        vk::PipelineStageFlagBits srcStage;
        vk::PipelineStageFlagBits dstStage;

        // Specify transition support. See https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            memoryBarrier.srcAccessMask = vk::AccessFlags();
            memoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
            dstStage = vk::PipelineStageFlagBits::eTransfer;

            queue = &device->transferQueue;
            commandPool = &context->transferCommandPool;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            memoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            memoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            srcStage = vk::PipelineStageFlagBits::eTransfer;
            dstStage = vk::PipelineStageFlagBits::eFragmentShader;

            queue = &device->graphicsQueue;
            commandPool = &context->graphicsCommandPool;
        }
        else
        {
            throw std::invalid_argument("Unsupported layout transition.");
        }

        auto commandBuffers = commandPool->beginSingleTimeCommands();

        commandBuffers[0].pipelineBarrier(
            srcStage, dstStage,
            vk::DependencyFlags(),
            nullptr,
            nullptr,
            memoryBarrier);

        commandPool->endSingleTimeCommands(commandBuffers, *queue);
    }

    /* helper function to create image views
        * @note: TODO: Should this be somewere else ? It doesn't depend on image members at all and is called from other places. 
        * If so what would be a good place ? Inside device ?
        */
    static vk::ImageView createImageView(std::shared_ptr<core::Device> device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels, vk::ImageViewType viewtype = vk::ImageViewType::e2D, int layerCount = 1)
    {
        vk::ImageViewCreateInfo createInfo;
        createInfo.format = format;
        createInfo.image = image;
        createInfo.viewType = viewtype;
        createInfo.subresourceRange.aspectMask = aspectMask;
        createInfo.subresourceRange.layerCount = layerCount;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.levelCount = mipLevels;
        createInfo.subresourceRange.baseMipLevel = 0;

        return device->logicalDevice.createImageView(createInfo);
    }

private:
};
} // namespace core