#include "image.hpp"
#include "allocation.hpp"
#include "context.hpp"
#include "device.hpp"
#include <fstream>
#include <memory>
#include <vulkan/vulkan.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace core
{

// Minimal Cpp wrapper for STB loader
// TODO by order of preference :
// 1) Get rid of it
// 2) Find it a better name
// 3) Move it somewhere else
stbi_uc* pixels;
int width, height, channels;

ImageFile::ImageFile(std::string path)
{
    load(path);
}

void ImageFile::load(std::string path)
{
    pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels)
    {
        throw std::runtime_error("Failed to load image at " + path);
    }
}

void Image::initImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
                      vk::ImageUsageFlags usage, int arrayLayers, vk::ImageCreateFlagBits flags, vk::ImageLayout layout)
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
    imageInfo.initialLayout = layout; // The very first iteration will discard the texels
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = numSamples;

    image = device->logical->createImageUnique(imageInfo);

    this->layout = layout;
    this->mipLevels = mipLevels;
    this->format = format;
    this->arrayLayers = arrayLayers;
    this->width = width;
    this->height = height;
}

void Image::allocate(const vk::MemoryPropertyFlags& memProperties)
{
    vk::MemoryRequirements memRequirements = device->logical->getImageMemoryRequirements(image.get());
    Allocation::allocate(memRequirements, memProperties);
    device->logical->bindImageMemory(image.get(), memory.get(), 0);
}

void Image::initView(vk::Format format, vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype)
{
    assert(!view && "Image view is already initialized.");
    view = createImageViewUnique(device, image.get(), format, aspectMask, mipLevels, viewtype, arrayLayers);
}

Image::Image() {}

// TODO: Default arguments
Image::Image(std::shared_ptr<core::Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
             vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask, vk::ImageLayout layout)
{

    this->device = device;

    initImage(width, height, mipLevels, numSamples, format, tiling, usage, 1, {}, layout);
    allocate(memProperties);
    initView(format, aspectMask);
}

// Create an image without a view.
Image::Image(std::shared_ptr<core::Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
             vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageLayout layout)
{
    this->device = device;

    initImage(width, height, mipLevels, numSamples, format, tiling, usage, 1, {}, layout);
    allocate(memProperties);
}

Image::operator vk::Image() { return image.get(); } // TODO: This is prone to confusion, get rid of it.

void Image::transitionLayout(vk::CommandBuffer cb, vk::ImageLayout newLayout)
{
    if (layout == newLayout)
        return;

    vk::ImageMemoryBarrier memoryBarrier;
    memoryBarrier.oldLayout = layout;
    memoryBarrier.newLayout = newLayout;
    memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.image = image.get();
    memoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    memoryBarrier.subresourceRange.layerCount = arrayLayers;
    memoryBarrier.subresourceRange.baseArrayLayer = 0;
    memoryBarrier.subresourceRange.levelCount = mipLevels;
    memoryBarrier.subresourceRange.baseMipLevel = 0;
    memoryBarrier.srcAccessMask = vk::AccessFlags(); // Which operations must happen before the barrier
    memoryBarrier.dstAccessMask = vk::AccessFlags(); // ... and after

    vk::PipelineStageFlagBits srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::PipelineStageFlagBits dstStage = vk::PipelineStageFlagBits::eTopOfPipe;

    // Specify transition support. See https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported
    // TODO: Refactor. This is quite verbose
    // TODO: Fill out reminding transitions
    switch (layout)
    {
    case vk::ImageLayout::eUndefined:
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        break;
    case vk::ImageLayout::eGeneral:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        srcStage = vk::PipelineStageFlagBits::eTransfer;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        srcStage = vk::PipelineStageFlagBits::eTransfer;
        break;
    case vk::ImageLayout::ePreinitialized:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eHostWrite;
        break;
    case vk::ImageLayout::ePresentSrcKHR:
        memoryBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
        break;
    }

    switch (newLayout)
    {
    case vk::ImageLayout::eUndefined:
        break;
    case vk::ImageLayout::eGeneral:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite; // TODO: MemoryRead ?
        dstStage = vk::PipelineStageFlagBits::eTransfer;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead; // TODO: eMemoryRead ?
        dstStage = vk::PipelineStageFlagBits::eTransfer;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
        break;
    case vk::ImageLayout::ePreinitialized:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::ePresentSrcKHR:
        memoryBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
        break;
    }

    cb.pipelineBarrier(
        srcStage, dstStage,
        vk::DependencyFlags(),
        nullptr,
        nullptr,
        memoryBarrier);

    this->layout = newLayout;
}

void Image::blit(vk::CommandBuffer cb, core::Image& dstImage)
{
    blit(cb, dstImage, width, height);
}

void Image::blit(vk::CommandBuffer cb, core::Image& dstImage, int width, int height)
{
    vk::Offset3D blitSize;
    blitSize.x = width;
    blitSize.y = height;
    blitSize.z = 1;
    vk::ImageBlit imageBlitRegion;
    imageBlitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = blitSize;
    imageBlitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = blitSize;

    cb.blitImage(
        image.get(), vk::ImageLayout::eTransferSrcOptimal,
        dstImage.image.get(), vk::ImageLayout::eTransferDstOptimal,
        imageBlitRegion,
        vk::Filter::eNearest);
}

void Image::copyTo(vk::CommandBuffer cb, core::Image& dstImage)
{
    copyTo(cb, dstImage, width, height);
}

void Image::copyTo(vk::CommandBuffer cb, core::Image& dstImage, int width, int height)
{
    vk::ImageCopy imageCopyRegion;
    imageCopyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = width;
    imageCopyRegion.extent.height = height;
    imageCopyRegion.extent.depth = 1;

    cb.copyImage(
        image.get(), vk::ImageLayout::eTransferSrcOptimal,
        dstImage.image.get(), vk::ImageLayout::eTransferDstOptimal,
        imageCopyRegion);
}

// Save image on disk as a ppm file.
// FIXME: This only works in 8-bits per channel formats
void Image::save(std::string filename, bool colorSwizzle)
{
    // TODO: Swizzle or not based on format
    vk::ImageSubresource subresource = {vk::ImageAspectFlagBits::eColor, 0, 0};
    auto subResourceLayout = device->logical->getImageSubresourceLayout(image.get(), subresource);

    if (mapped == nullptr)
    {
        map();
    }
    char* data = static_cast<char*>(mapped);
    data += subResourceLayout.offset;

    std::ofstream file(filename, std::ios::out | std::ios::binary);

    // ppm header
    file << "P6\n"
         << width << "\n"
         << height << "\n"
         << 255 << "\n";

    for (uint32_t y = 0; y < height; y++)
    {
        unsigned int* row = (unsigned int*) data;
        for (uint32_t x = 0; x < width; x++)
        {
            if (colorSwizzle)
            {
                file.write((char*) row + 2, 1);
                file.write((char*) row + 1, 1);
                file.write((char*) row, 1);
            }
            else
            {
                file.write((char*) row, 3);
            }
            row++;
        }
        data += subResourceLayout.rowPitch;
    }

    file.close();
}

// Retreive the pixel value at index
// FIXME: This won't work if the image is in GPU-specific format
// FIXME: This only works in 8-bits per channel formats
glm::vec3 Image::pixelAt(int x, int y, bool colorSwizzle)
{
    vk::ImageSubresource subresource = {vk::ImageAspectFlagBits::eColor, 0, 0};
    auto subResourceLayout = device->logical->getImageSubresourceLayout(image.get(), subresource);

    if (mapped == nullptr)
    {
        map();
    }
    char* data = static_cast<char*>(mapped);
    data += subResourceLayout.offset;

    // TODO: Offsets depend on image format
    // TODO: Get rid of the loops
    for (uint32_t iy = 0; iy < height; iy++)
    {
        unsigned int* row = (unsigned int*) data;
        for (uint32_t ix = 0; ix < width; ix++)
        {
            uint8_t* pixel = (uint8_t*) row;
            if (ix == x && iy == y)
            {
                // FIXME: Color swizzle is not set properly.
                if (colorSwizzle)
                {
                    std::cout << "R: " << (int) *(pixel + 2) << " G: " << (int) *(pixel + 1) << " B: " << (int) *pixel << std::endl;
                    int r = (unsigned int) *(pixel + 2);
                    int g = (unsigned int) *(pixel + 1);
                    int b = (unsigned int) *pixel;
                    return glm::vec3(r, g, b);
                }
                else
                {
                    std::cout << "R: " << (int) *pixel << " G: " << (int) *(pixel + 1) << " B: " << (int) *(pixel + 2) << std::endl;
                    int r = (unsigned int) *pixel;
                    int g = (unsigned int) *(pixel + 1);
                    int b = (unsigned int) *(pixel + 2);
                    return glm::vec3(r, g, b);
                }
            }
            row++;
        }
        data += subResourceLayout.rowPitch;
    }
    return glm::vec3();
}

// Helper function to create image views
// @note: TODO: Should this be somewere else ? It doesn't depend on image members at all and is called from other places.
// If so what would be a good place ? Inside device ?
//
vk::ImageView Image::createImageView(std::shared_ptr<core::Device> device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels, vk::ImageViewType viewtype, int layerCount)
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

    return device->logical->createImageView(createInfo);
}

vk::UniqueImageView Image::createImageViewUnique(std::shared_ptr<core::Device> device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels, vk::ImageViewType viewtype, int layerCount)
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

    return std::move(device->logical->createImageViewUnique(createInfo));
}
} // namespace core