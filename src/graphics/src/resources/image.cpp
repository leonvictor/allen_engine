#include "resources/image.hpp"

#include "render_engine.hpp"
#include "resources/buffer.hpp"

#include <common/containers/array.hpp>
#include <config/path.h>

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace aln::resources
{

void Image::Initialize(RenderEngine* pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, int arrayLayers, vk::ImageCreateFlagBits flags, vk::ImageLayout layout, vk::ImageType type)
{
    m_pRenderEngine = pDevice;
    InitImage(width, height, mipLevels, numSamples, format, tiling, usage, arrayLayers, flags, layout, type);
}

void Image::Initialize(RenderEngine* pDevice, vk::Image& image, vk::Format format)
{
    m_pRenderEngine = pDevice;
    m_externallyOwnedImage = true;
    m_image = image;
    m_format = format;
}

void Image::InitImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, uint32_t arrayLayers, vk::ImageCreateFlagBits flags, vk::ImageLayout layout, vk::ImageType type)
{
    // Enforce vk specs
    assert(layout == vk::ImageLayout::eUndefined || layout == vk::ImageLayout::ePreinitialized);

    vk::ImageCreateInfo imageInfo = {
        .flags = flags,
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = mipLevels,
        .arrayLayers = arrayLayers,
        .samples = numSamples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = layout,
    };

    m_image = m_pRenderEngine->GetVkDevice().createImage(imageInfo).value;

#ifndef NDEBUG
    m_pRenderEngine->SetDebugUtilsObjectName(m_image, m_debugName + " Image");
#endif

    m_layout = layout;
    m_mipLevels = mipLevels;
    m_format = format;
    m_arrayLayers = arrayLayers;
    m_width = width;
    m_height = height;
}

void Image::Shutdown()
{
    m_pRenderEngine->GetVkDevice().destroySampler(m_sampler);
    m_pRenderEngine->GetVkDevice().destroyImageView(m_view);

    if (!m_externallyOwnedImage)
    {
        m_pRenderEngine->GetVkDevice().destroyImage(m_image);
    }

    Allocation::Shutdown();
}

void Image::AddView(vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype)
{
    assert(!m_view && "Image view is already initialized.");

    vk::ImageViewCreateInfo createInfo = {
        .image = m_image,
        .viewType = viewtype,
        .format = m_format,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = m_mipLevels,
            .baseArrayLayer = 0,
            .layerCount = m_arrayLayers,
        },
    };

    m_view = m_pRenderEngine->GetVkDevice().createImageView(createInfo).value;

#ifndef NDEBUG
    m_pRenderEngine->SetDebugUtilsObjectName(m_view, m_debugName + " Image View");
#endif
}

void Image::Allocate(const vk::MemoryPropertyFlags& memProperties)
{
    auto memRequirements = m_pRenderEngine->GetVkDevice().getImageMemoryRequirements(m_image);
    Allocation::Allocate(memRequirements, memProperties);
    m_pRenderEngine->GetVkDevice().bindImageMemory(m_image, m_memory, 0);
}

void Image::AddSampler(vk::SamplerAddressMode adressMode)
{
    vk::SamplerCreateInfo samplerInfo = {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = adressMode,
        .addressModeV = adressMode,
        .addressModeW = adressMode,
        .mipLodBias = 0,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = 16,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = (float) m_mipLevels,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False,
    };

    m_sampler = m_pRenderEngine->GetVkDevice().createSampler(samplerInfo).value;

#ifndef NDEBUG
    m_pRenderEngine->SetDebugUtilsObjectName(m_sampler, m_debugName + " Sampler");
#endif
}

void Image::TransitionLayout(vk::CommandBuffer cb, vk::ImageLayout newLayout)
{
    if (m_layout == newLayout)
    {
        return;
    }

    vk::ImageMemoryBarrier memoryBarrier =
        {
            .srcAccessMask = {},
            .dstAccessMask = {},
            .oldLayout = m_layout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = m_image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = m_mipLevels,
                .baseArrayLayer = 0,
                .layerCount = m_arrayLayers,
            },
        };

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

void Image::Blit(vk::CommandBuffer cb, Image& dstImage)
{
    Blit(cb, dstImage, m_width, m_height);
}

void Image::Blit(vk::CommandBuffer cb, Image& dstImage, uint32_t width, uint32_t height)
{
    vk::Offset3D blitSize = {width, height, 1};
    vk::Offset3D defaultOffset = {0, 0, 0};
    vk::ImageBlit imageBlitRegion = {
        .srcSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .layerCount = 1,
        },
        .srcOffsets = {{defaultOffset, blitSize}},
        .dstSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .layerCount = 1,
        },
        .dstOffsets = {{defaultOffset, blitSize}},
    };

    cb.blitImage(
        m_image, vk::ImageLayout::eTransferSrcOptimal,
        dstImage.m_image, vk::ImageLayout::eTransferDstOptimal,
        imageBlitRegion,
        vk::Filter::eNearest);
}

void Image::CopyTo(vk::CommandBuffer cb, Image& dstImage)
{
    CopyTo(cb, dstImage, m_width, m_height);
}

void Image::CopyTo(vk::CommandBuffer cb, Image& dstImage, uint32_t width, uint32_t height)
{
    vk::ImageCopy imageCopyRegion = {
        .srcSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .layerCount = 1,
        },
        .dstSubresource = {

            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .layerCount = 1,
        },
        .extent = {
            .width = width,
            .height = height,
            .depth = 1,
        },
    };

    cb.copyImage(
        m_image, vk::ImageLayout::eTransferSrcOptimal,
        dstImage.m_image, vk::ImageLayout::eTransferDstOptimal,
        imageCopyRegion);
}

// Save image on disk as a ppm file.
// FIXME: This only works in 8-bits per channel formats
void Image::Save(std::string filename, bool colorSwizzle)
{
    // TODO: Swizzle or not based on format
    vk::ImageSubresource subresource = {vk::ImageAspectFlagBits::eColor, 0, 0};
    auto subResourceLayout = m_pRenderEngine->GetVkDevice().getImageSubresourceLayout(m_image, subresource);

    if (m_mapped == nullptr)
    {
        Map();
    }
    char* data = static_cast<char*>(m_mapped);
    data += subResourceLayout.offset;

    std::ofstream file(filename, std::ios::out | std::ios::binary);

    // ppm header
    file << "P6\n"
         << m_width << "\n"
         << m_height << "\n"
         << 255 << "\n";

    for (uint32_t y = 0; y < m_height; y++)
    {
        unsigned int* row = (unsigned int*) data;
        for (uint32_t x = 0; x < m_width; x++)
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
Vec3 Image::PixelAt(uint32_t x, uint32_t y, bool colorSwizzle)
{
    vk::ImageSubresource subresource = {vk::ImageAspectFlagBits::eColor, 0, 0};
    auto subResourceLayout = m_pRenderEngine->GetVkDevice().getImageSubresourceLayout(m_image, subresource);

    if (m_mapped == nullptr)
    {
        Map();
    }
    char* data = static_cast<char*>(m_mapped);
    data += subResourceLayout.offset;

    // TODO: Offsets depend on image format
    // TODO: Get rid of the loops
    for (uint32_t iy = 0; iy < m_height; iy++)
    {
        unsigned int* row = (unsigned int*) data;
        for (uint32_t ix = 0; ix < m_width; ix++)
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
                    return Vec3(r, g, b);
                }
                else
                {
                    std::cout << "R: " << (int) *pixel << " G: " << (int) *(pixel + 1) << " B: " << (int) *(pixel + 2) << std::endl;
                    int r = (unsigned int) *pixel;
                    int g = (unsigned int) *(pixel + 1);
                    int b = (unsigned int) *(pixel + 2);
                    return Vec3(r, g, b);
                }
            }
            row++;
        }
        data += subResourceLayout.rowPitch;
    }
    return Vec3();
}

void Image::GenerateMipMaps(vk::CommandBuffer& cb, uint32_t mipLevels)
{
    auto formatProperties = m_pRenderEngine->GetFormatProperties(m_format);

    vk::ImageMemoryBarrier barrier = {
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = m_image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    int32_t mipWidth = m_width;
    int32_t mipHeight = m_height;

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
        vk::ImageBlit blit = {
            .srcSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffsets = {{vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth, mipHeight, 1}}},
            .dstSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = {{vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}}},
        };

        cb.blitImage(
            m_image, vk::ImageLayout::eTransferSrcOptimal,
            m_image, vk::ImageLayout::eTransferDstOptimal,
            blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cb.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            barrier);

        // Handle cases where the image is not square
        if (mipWidth > 1)
        {
            mipWidth /= 2;
        }

        if (mipHeight > 1)
        {
            mipHeight /= 2;
        }
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    // Transition the last mip level to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    cb.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {},
        nullptr,
        nullptr,
        barrier);

    m_mipLevels = mipLevels;
    m_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

const vk::DescriptorSet& Image::GetDescriptorSet() const
{
    assert(m_descriptorSet);
    return m_descriptorSet;
}

void Image::CreateDescriptorSet()
{
    // Lazy allocation of the descriptor set
    assert(!m_descriptorSet);

    m_descriptorSet = m_pRenderEngine->AllocateDescriptorSet<Image>();

    // Update the Descriptor Set:
    auto desc = GetDescriptor();

    vk::WriteDescriptorSet writeDesc[1] = {{
        .dstSet = m_descriptorSet,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &desc,
    }};

    m_pRenderEngine->GetVkDevice().updateDescriptorSets(1, writeDesc, 0, nullptr);
    m_pRenderEngine->SetDebugUtilsObjectName(m_descriptorSet, m_debugName + " Descriptor Set");
}

Vector<vk::DescriptorSetLayoutBinding> Image::GetDescriptorSetLayoutBindings()
{
    // Is it possible do get a descriptor for a non-sampled image ?
    Vector<vk::DescriptorSetLayoutBinding> bindings = {
        {0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
    };

    return bindings;
}

void Image::InitializeFromBuffer(RenderEngine* pDevice, vk::CommandBuffer& cb, Buffer& buffer, uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, uint32_t arrayLayers, vk::ImageType type)
{
    Initialize(pDevice, width, height, mipLevels,
        vk::SampleCountFlagBits::e1, format, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
        arrayLayers, {}, vk::ImageLayout::eUndefined, type);

    Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);

    TransitionLayout(cb, vk::ImageLayout::eTransferDstOptimal);
    buffer.CopyTo(cb, *this);

    // Optionnaly generate mipmaps
    if (mipLevels > 1)
    {
        GenerateMipMaps(cb, mipLevels);
    }
    else
    {
        TransitionLayout(cb, vk::ImageLayout::eShaderReadOnlyOptimal);
    }
}

// void Image::InitializeFromCubemapFromDirectory(RenderEngine* pDevice, std::string path)
//{
//     // TODO: Quick and dirty way of storing faces names for now
//     // Generate optimized file ?
//     Array<std::string, 6> faces = {
//         "Right",
//         "Left",
//         "Up",
//         "Down",
//         "Front",
//         "Back",
//     };
//
//     // TODO: Remove hardcoded path to textures
//     // TODO: Load the first face before the loop to initialize buffers
//     auto facePath = std::string(DEFAULT_ASSETS_DIR) + "/skyboxes/midday/CloudyCrown_Midday_" + faces[0] + ".png";
//     int width, height, channels;
//     stbi_uc* pixels = stbi_load(facePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
//
//     if (!pixels)
//     {
//         std::cout << "Failed to load texture file " << facePath << std::endl;
//         auto why = stbi_failure_reason();
//         std::cout << why << std::endl;
//         throw;
//     }
//
//     vk::DeviceSize faceSize = width * height * channels;
//     Buffer stagingBuffer;
//     stagingBuffer.Initialize(pDevice, faceSize * 6, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
//
//     stagingBuffer.Map(0, faceSize);
//     stagingBuffer.Copy(pixels, static_cast<size_t>(faceSize));
//     stagingBuffer.Unmap();
//
//     vk::BufferImageCopy bufferImageCopy = {
//         .bufferOffset = 0,
//         .imageSubresource = {
//             .aspectMask = vk::ImageAspectFlagBits::eColor,
//             .mipLevel = 0,
//             .baseArrayLayer = 0,
//             .layerCount = 1,
//         },
//         .imageExtent = {
//             .width = static_cast<uint32_t>(width),
//             .height = static_cast<uint32_t>(height),
//             .depth = 1,
//         },
//     };
//
//     Vector<vk::BufferImageCopy> bufferCopyRegions;
//     bufferCopyRegions.push_back(bufferImageCopy);
//
//     for (uint32_t i = 1; i < faces.size(); i++)
//     {
//         facePath = std::string(DEFAULT_ASSETS_DIR) + "/skyboxes/midday/CloudyCrown_Midday_" + faces[i] + ".png";
//
//         pixels = stbi_load(facePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
//
//         if (!pixels)
//         {
//             std::cout << "Failed to load texture file " << path << std::endl;
//             throw;
//         }
//
//         // Copy data to staging buffer
//         stagingBuffer.Map(i * faceSize, faceSize);
//         stagingBuffer.Copy(pixels, static_cast<size_t>(faceSize));
//         stagingBuffer.Unmap();
//
//         bufferImageCopy.bufferOffset = faceSize * i;
//         bufferImageCopy.imageSubresource.mipLevel = 0;
//         bufferImageCopy.imageSubresource.baseArrayLayer = i;
//         // bufferImageCopy.imageExtent = {width, height};
//         bufferCopyRegions.push_back(bufferImageCopy);
//     }
//
//     Initialize(pDevice, width, height, 1,
//         vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
//         vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
//         6, vk::ImageCreateFlagBits::eCubeCompatible, vk::ImageLayout::eUndefined, vk::ImageType::e3D);
//
//     Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
//
//     pDevice->GetGraphicsCommandPool().Execute([&](vk::CommandBuffer cb)
//         {
//             TransitionLayout(cb, vk::ImageLayout::eTransferDstOptimal);
//             stagingBuffer.CopyTo(cb, *this);
//             TransitionLayout(cb, vk::ImageLayout::eShaderReadOnlyOptimal); });
// }

/// @brief [DEBUG ONLY] Set a debug name to this image and all its underlying vulkan objects.
/// @todo Make sure this is a noop in release.
void Image::SetDebugName(std::string name)
{
#ifndef NDEBUG
    m_debugName = name;
    if (m_image)
    {
        m_pRenderEngine->SetDebugUtilsObjectName(m_image, name + " Image");
    }
    if (m_view)
    {
        m_pRenderEngine->SetDebugUtilsObjectName(m_view, name + " Image View");
    }
    if (m_sampler)
    {
        m_pRenderEngine->SetDebugUtilsObjectName(m_sampler, name + " Sampler");
    }
    if (m_descriptorSet)
    {
        m_pRenderEngine->SetDebugUtilsObjectName(m_descriptorSet, name + " Descriptor Set");
    }
#endif
}
} // namespace aln::resources