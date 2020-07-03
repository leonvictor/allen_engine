#include "texture.hpp"
#include "buffer.hpp"
#include "context.hpp"
#include "device.hpp"
#include "image.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace core
{
void Texture::createSampler(vk::SamplerAddressMode adressMode)
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
    samplerInfo.maxLod = static_cast<uint32_t>(mipLevels);
    samplerInfo.minLod = 0;

    sampler = device->logical.get().createSampler(samplerInfo);
}

Texture::Texture() {}

Texture::Texture(std::shared_ptr<core::Context> context, std::string path)
{
    this->context = context;

    createTextureImage(context, path);
    createSampler();
}

void Texture::destroy()
{
    device->logical.get().destroySampler(sampler);
    core::Image::destroy();
}

// TODO: This could come from a Descriptible interface (common w/ buffers)
vk::DescriptorImageInfo Texture::getDescriptor()
{
    return vk::DescriptorImageInfo{sampler, view.get(), vk::ImageLayout::eShaderReadOnlyOptimal};
}

void Texture::createTextureImage(std::shared_ptr<core::Context> context, std::string path)
{
    this->device = context->device;
    this->context = context;

    // Load image from file
    core::ImageFile img = core::ImageFile(path);

    // Copy data to staging buffer
    vk::DeviceSize imageSize = img.width * img.height * 4;
    core::Buffer stagingBuffer(device, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.map(0, imageSize);
    stagingBuffer.copy(img.pixels, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    // TODO: Move mipmaps generation out. Do we *need* it to happen before view creation ? We can also recreate the view
    // TODO: Can we change the "mipLevel" field of an image on the fly (to initialize it at 1 here)
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(img.width, img.height)))) + 1;

    initImage(img.width, img.height, mipLevels, vk::SampleCountFlagBits::e1,
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc);

    allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Transition the image to transfer dst layout
    context->device->commandpools.transfer.execute([&](vk::CommandBuffer cb) {
        transitionLayout(cb, vk::ImageLayout::eTransferDstOptimal);
        stagingBuffer.copyTo(cb, image.get(), img.width, img.height);
    });

    generateMipMaps(image.get(), vk::Format::eR8G8B8A8Srgb, img.width, img.height, mipLevels);

    initView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

    stagingBuffer.destroy();
}

// TODO :
// - Move to image ? It's weird as long as we need context
//
void Texture::generateMipMaps(vk::Image image, vk::Format format, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels)
{

    auto formatProperties = device->physical.getFormatProperties(format);
    auto commandBuffers = context->device->commandpools.graphics.beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier;
    barrier.image = image;
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

        commandBuffers[0].pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags(),
            nullptr,
            nullptr,
            barrier);

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

        commandBuffers[0].blitImage(
            image, vk::ImageLayout::eTransferSrcOptimal,
            image, vk::ImageLayout::eTransferDstOptimal,
            blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffers[0].pipelineBarrier(
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
    commandBuffers[0].pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlags(),
        nullptr,
        nullptr,
        barrier);

    context->device->commandpools.graphics.endSingleTimeCommands(commandBuffers); // TODO: Use simpler API
}
} // namespace core