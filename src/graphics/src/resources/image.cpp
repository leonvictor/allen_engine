#include "resources/image.hpp"
#include "resources/allocation.hpp"
#include "resources/buffer.hpp"

#include <config/path.h>

#include <cmath>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace aln::vkg::resources
{

Image::Image(Device* pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, int arrayLayers, vk::ImageCreateFlagBits flags, vk::ImageLayout layout, vk::ImageType type)
{
    m_pDevice = pDevice;
    InitImage(width, height, mipLevels, numSamples, format, tiling, usage, arrayLayers, flags, layout, type);
}

Image::Image(Device* pDevice, vk::Image& image, vk::Format format)
{
    m_pDevice = pDevice;
    m_externallyOwnedImage = true;
    m_vkImage = vk::UniqueImage(image);
    m_format = format;
}

void Image::InitImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, int arrayLayers, vk::ImageCreateFlagBits flags, vk::ImageLayout layout, vk::ImageType type)
{
    // Enforce vk specs
    assert(layout == vk::ImageLayout::eUndefined || layout == vk::ImageLayout::ePreinitialized);

    vk::ImageCreateInfo imageInfo;
    imageInfo.flags = flags;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D(width, height, 1);
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = layout; // The very first iteration will discard the texel;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = numSamples;

    m_vkImage = m_pDevice->GetVkDevice().createImageUnique(imageInfo);

#ifndef NDEBUG
    m_pDevice->SetDebugUtilsObjectName(m_vkImage.get(), m_debugName + " Image");
#endif

    m_layout = layout;
    m_mipLevels = mipLevels;
    m_format = format;
    m_arrayLayers = arrayLayers;
    m_width = width;
    m_height = height;
}

void Image::AddView(vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype)
{
    assert(!m_vkView && "Image view is already initialized.");

    vk::ImageViewCreateInfo createInfo;
    createInfo.format = m_format;
    createInfo.image = m_vkImage.get();
    createInfo.viewType = viewtype;
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.layerCount = m_arrayLayers;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.levelCount = m_mipLevels;
    createInfo.subresourceRange.baseMipLevel = 0;

    m_vkView = m_pDevice->GetVkDevice().createImageViewUnique(createInfo);

#ifndef NDEBUG
    m_pDevice->SetDebugUtilsObjectName(m_vkView.get(), m_debugName + " Image View");
#endif
}

void Image::Allocate(const vk::MemoryPropertyFlags& memProperties)
{
    auto memRequirements = m_pDevice->GetVkDevice().getImageMemoryRequirements(m_vkImage.get());
    Allocation::Allocate(memRequirements, memProperties);
    m_pDevice->GetVkDevice().bindImageMemory(m_vkImage.get(), m_memory.get(), 0);
}

void Image::AddSampler(vk::SamplerAddressMode adressMode)
{
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eLinear; // How to interpolate texels that are magnified...
    samplerInfo.minFilter = vk::Filter::eLinear; // or minified
    // Addressing mode per axis
    samplerInfo.addressModeU = adressMode; // x
    samplerInfo.addressModeV = adressMode; // y
    samplerInfo.addressModeW = adressMode; // z
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0;
    samplerInfo.maxLod = static_cast<uint32_t>(m_mipLevels);
    samplerInfo.minLod = 0;

    m_vkSampler = m_pDevice->GetVkDevice().createSamplerUnique(samplerInfo);

#ifndef NDEBUG
    m_pDevice->SetDebugUtilsObjectName(m_vkSampler.get(), m_debugName + " Sampler");
#endif
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

void Image::Blit(vk::CommandBuffer cb, Image& dstImage)
{
    Blit(cb, dstImage, m_width, m_height);
}

void Image::Blit(vk::CommandBuffer cb, Image& dstImage, int width, int height)
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

void Image::CopyTo(vk::CommandBuffer cb, Image& dstImage)
{
    CopyTo(cb, dstImage, m_width, m_height);
}

void Image::CopyTo(vk::CommandBuffer cb, Image& dstImage, int width, int height)
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

void Image::GenerateMipMaps(vk::CommandBuffer& cb, uint32_t mipLevels)
{
    auto formatProperties = m_pDevice->GetFormatProperties(m_format);

    vk::ImageMemoryBarrier barrier;
    barrier.image = m_vkImage.get();
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

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
    m_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

vk::DescriptorSet& Image::GetDescriptorSet()
{
    // Lazy allocation of the descriptor set
    if (!m_vkDescriptorSet)
    {
        m_vkDescriptorSet = m_pDevice->AllocateDescriptorSet<Image>();

        // Update the Descriptor Set:
        vk::WriteDescriptorSet writeDesc[1] = {};
        writeDesc[0].dstSet = m_vkDescriptorSet.get();
        writeDesc[0].descriptorCount = 1;
        writeDesc[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        auto desc = GetDescriptor();
        writeDesc[0].pImageInfo = &desc;

        m_pDevice->GetVkDevice().updateDescriptorSets(1, writeDesc, 0, nullptr);

#ifndef NDEBUG
        m_pDevice->SetDebugUtilsObjectName(m_vkDescriptorSet.get(), m_debugName + " Descriptor Set");
#endif
    }
    return m_vkDescriptorSet.get();
}

std::vector<vk::DescriptorSetLayoutBinding> Image::GetDescriptorSetLayoutBindings()
{
    // Is it possible do get a descriptor for a non-sampled image ?
    std::vector<vk::DescriptorSetLayoutBinding> bindings{
        {0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
    };

    return bindings;
}

Image Image::FromBuffer(Device* pDevice, Buffer& buffer, uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, uint32_t arrayLayers, vk::ImageType type)
{
    Image image = Image(pDevice, width, height, mipLevels,
        vk::SampleCountFlagBits::e1, format, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
        arrayLayers, {}, vk::ImageLayout::eUndefined, type);

    image.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);

    pDevice->GetGraphicsCommandPool()
        .Execute([&](vk::CommandBuffer cb)
            {
                image.TransitionLayout(cb, vk::ImageLayout::eTransferDstOptimal);
                buffer.CopyTo(cb, image);
                // Optionnaly generate mipmaps
                if (mipLevels > 1)
                {

                    image.GenerateMipMaps(cb, mipLevels);
                }
                else
                {
                    image.TransitionLayout(cb, vk::ImageLayout::eShaderReadOnlyOptimal);
                } });

    return std::move(image);
}

Image Image::CubemapFromDirectory(Device* pDevice, std::string path)
{
    // TODO: Quick and dirty way of storing faces names for now
    // Generate optimized file ?
    std::array<std::string, 6> faces = {
        "Right",
        "Left",
        "Up",
        "Down",
        "Front",
        "Back",
    };

    // TODO: Remove hardcoded path to textures
    // TODO: Load the first face before the loop to initialize buffers
    auto facePath = std::string(DEFAULT_ASSETS_DIR) + "/skyboxes/midday/CloudyCrown_Midday_" + faces[0] + ".png";
    int width, height, channels;
    stbi_uc* pixels = stbi_load(facePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels)
    {
        std::cout << "Failed to load texture file " << facePath << std::endl;
        auto why = stbi_failure_reason();
        std::cout << why << std::endl;
        throw;
    }

    vk::DeviceSize faceSize = width * height * channels;
    Buffer stagingBuffer = Buffer(pDevice, faceSize * 6, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.Map(0, faceSize);
    stagingBuffer.Copy(pixels, static_cast<size_t>(faceSize));
    stagingBuffer.Unmap();

    std::vector<vk::BufferImageCopy> bufferCopyRegions;

    vk::BufferImageCopy bufferImageCopy;
    bufferImageCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    bufferImageCopy.imageSubresource.layerCount = 1;

    bufferImageCopy.imageExtent.depth = 1;
    bufferImageCopy.imageExtent.width = width;
    bufferImageCopy.imageExtent.height = height;

    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferCopyRegions.push_back(bufferImageCopy);

    for (uint32_t i = 1; i < faces.size(); i++)
    {
        facePath = std::string(DEFAULT_ASSETS_DIR) + "/skyboxes/midday/CloudyCrown_Midday_" + faces[i] + ".png";

        pixels = stbi_load(facePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels)
        {
            std::cout << "Failed to load texture file " << path << std::endl;
            throw;
        }

        // Copy data to staging buffer
        stagingBuffer.Map(i * faceSize, faceSize);
        stagingBuffer.Copy(pixels, static_cast<size_t>(faceSize));
        stagingBuffer.Unmap();

        bufferImageCopy.bufferOffset = faceSize * i;
        bufferImageCopy.imageSubresource.mipLevel = 0;
        bufferImageCopy.imageSubresource.baseArrayLayer = i;
        // bufferImageCopy.imageExtent = {width, height};
        bufferCopyRegions.push_back(bufferImageCopy);
    }

    Image image = Image(pDevice, width, height, 1,
        vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
        6, vk::ImageCreateFlagBits::eCubeCompatible, vk::ImageLayout::eUndefined, vk::ImageType::e3D);

    image.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);

    pDevice->GetGraphicsCommandPool().Execute([&](vk::CommandBuffer cb)
        {
            image.TransitionLayout(cb, vk::ImageLayout::eTransferDstOptimal);
            stagingBuffer.CopyTo(cb, image);
            image.TransitionLayout(cb, vk::ImageLayout::eShaderReadOnlyOptimal); });
    return std::move(image);
}
} // namespace aln::vkg::resources