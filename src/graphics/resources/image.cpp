#include "image.hpp"
#include "allocation.hpp"
#include <fstream>
#include <memory>

#include <vulkan/vulkan.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vkg
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

void Image::InitImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
                      vk::ImageUsageFlags usage, int arrayLayers, vk::ImageCreateFlagBits flags, vk::ImageLayout layout)
{

    vk::ImageCreateInfo imageInfo;
    imageInfo.flags = flags;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .depth = 1,
    },
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = layout; // The very first iteration will discard the texel;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = numSamples;

    m_vkImage = m_pDevice->GetVkDevice().createImageUnique(imageInfo);

    m_layout = layout;
    m_mipLevels = mipLevels;
    m_format = format;
    m_arrayLayers = arrayLayers;
    m_width = width;
    m_height = height;
}

void Image::Allocate(const vk::MemoryPropertyFlags& memProperties)
{
    vk::MemoryRequirements memRequirements = m_pDevice->GetVkDevice().getImageMemoryRequirements(m_vkImage.get());
    Allocation::Allocate(memRequirements, memProperties);
    m_pDevice->GetVkDevice().bindImageMemory(m_vkImage.get(), m_memory.get(), 0);
}

void Image::InitView(vk::Format format, vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype)
{
    assert(!m_vkView && "Image view is already initialized.");

    vk::ImageViewCreateInfo createInfo;
    createInfo.format = format;
    createInfo.image = m_vkImage.get();
    createInfo.viewType = viewtype;
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.layerCount = m_arrayLayers;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.levelCount = m_mipLevels;
    createInfo.subresourceRange.baseMipLevel = 0;

    m_vkView = m_pDevice->GetVkDevice().createImageViewUnique(createInfo);
}

// TODO: Default arguments
Image::Image(std::shared_ptr<Device> pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
             vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask, vk::ImageLayout layout)
{

    m_pDevice = pDevice;

    InitImage(width, height, mipLevels, numSamples, format, tiling, usage, 1, {}, layout);
    Allocate(memProperties);
    InitView(format, aspectMask);
}

Image::Image(std::shared_ptr<Device> pDevice, vk::Image& image, vk::Format format, uint32_t mipLevels, vk::ImageAspectFlags aspectMask)
{
    m_pDevice = pDevice;
    m_vkImage = vk::UniqueImage(image);

    m_mipLevels = mipLevels;
    m_format = format;
    m_arrayLayers = 1;

    InitView(format, aspectMask);
}

void Image::TransitionLayout(vk::CommandBuffer cb, vk::ImageLayout newLayout)
{
    if (m_layout == newLayout)
        return;

    vk::ImageMemoryBarrier memoryBarrier;
    memoryBarrier.oldLayout = m_layout;
    memoryBarrier.newLayout = newLayout;
    memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.image = m_vkImage.get();
    memoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    memoryBarrier.subresourceRange.layerCount = m_arrayLayers;
    memoryBarrier.subresourceRange.baseArrayLayer = 0;
    memoryBarrier.subresourceRange.levelCount = m_mipLevels;
    memoryBarrier.subresourceRange.baseMipLevel = 0;
    memoryBarrier.srcAccessMask = vk::AccessFlags(); // Which operations must happen before the barrier
    memoryBarrier.dstAccessMask = vk::AccessFlags(); // ... and after

    vk::PipelineStageFlagBits srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::PipelineStageFlagBits dstStage = vk::PipelineStageFlagBits::eTopOfPipe;

    // Specify transition support. See https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported
    // TODO: Refactor. This is quite verbose
    // TODO: Fill out reminding transitions
    switch (m_layout)
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

    m_layout = newLayout;
}

void Image::Blit(vk::CommandBuffer cb, vkg::Image& dstImage)
{
    Blit(cb, dstImage, width, height);
}

void Image::Blit(vk::CommandBuffer cb, vkg::Image& dstImage, int width, int height)
{
    vk::Offset3D blitSize = {width, height, 1};

    vk::ImageBlit imageBlitRegion;
    imageBlitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = blitSize;
    imageBlitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = blitSize;

    cb.blitImage(
        m_vkImage.get(), vk::ImageLayout::eTransferSrcOptimal,
        dstImage.m_vkImage.get(), vk::ImageLayout::eTransferDstOptimal,
        imageBlitRegion,
        vk::Filter::eNearest);
}

void Image::CopyTo(vk::CommandBuffer cb, vkg::Image& dstImage)
{
    CopyTo(cb, dstImage, width, height);
}

void Image::CopyTo(vk::CommandBuffer cb, vkg::Image& dstImage, int width, int height)
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
        m_vkImage.get(), vk::ImageLayout::eTransferSrcOptimal,
        dstImage.m_vkImage.get(), vk::ImageLayout::eTransferDstOptimal,
        imageCopyRegion);
}

// Save image on disk as a ppm file.
// FIXME: This only works in 8-bits per channel formats
void Image::Save(std::string filename, bool colorSwizzle)
{
    // TODO: Swizzle or not based on format
    vk::ImageSubresource subresource = {vk::ImageAspectFlagBits::eColor, 0, 0};
    auto subResourceLayout = m_pDevice->GetVkDevice().getImageSubresourceLayout(m_vkImage.get(), subresource);

    if (m_mapped == nullptr)
    {
        Map();
    }
    char* data = static_cast<char*>(m_mapped);
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
glm::vec3 Image::PixelAt(int x, int y, bool colorSwizzle)
{
    vk::ImageSubresource subresource = {vk::ImageAspectFlagBits::eColor, 0, 0};
    auto subResourceLayout = m_pDevice->GetVkDevice().getImageSubresourceLayout(m_vkImage.get(), subresource);

    if (m_mapped == nullptr)
    {
        Map();
    }
    char* data = static_cast<char*>(m_mapped);
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

void Image::GenerateMipMaps(vk::CommandBuffer& cb, vk::Format format, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels)
{
    auto formatProperties = m_pDevice->GetFormatProperties(format);

    vk::ImageMemoryBarrier barrier;
    barrier.image = m_vkImage.get();
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cb.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags(),
            nullptr,
            nullptr,
            barrier);

        // TODO: Expand blit method to accept more parameters and use it here
        vk::ImageBlit blit;
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1}; // Where is the data that we will blit from ?
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.layerCount = 1;
        blit.dstSubresource.baseArrayLayer = 0;

        cb.blitImage(
            m_vkImage.get(), vk::ImageLayout::eTransferSrcOptimal,
            m_vkImage.get(), vk::ImageLayout::eTransferDstOptimal,
            blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cb.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlags(),
            nullptr,
            nullptr,
            barrier);

        // Handle cases where the image is not square
        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    // Transition the last mip level to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    cb.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlags(),
        nullptr,
        nullptr,
        barrier);

    m_mipLevels = mipLevels;
}
} // namespace vkg