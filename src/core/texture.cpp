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

    sampler = device->logical->createSamplerUnique(samplerInfo);
}

Texture::Texture() {}

Texture::Texture(std::shared_ptr<core::Context> context, std::string path)
{
    this->device = context->device;

    // Load image from file
    core::ImageFile img = core::ImageFile(path);

    // Copy data to staging buffer
    vk::DeviceSize imageSize = img.width * img.height * 4;
    core::Buffer stagingBuffer(device, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.map(0, imageSize);
    stagingBuffer.copy(img.pixels, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    createTextureImage(context, stagingBuffer, img.width, img.height);
    createSampler();
}

// Never used...
// Texture::Texture(std::shared_ptr<core::Context> context, void* buffer, vk::DeviceSize bufferSize, uint32_t texWidth, uint32_t texHeight)
// {
//     // TODO: Most parameters are not used
//     this->device = context->device;

//     core::Buffer stagingBuffer = core::Buffer(context->device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

//     stagingBuffer.map(0, bufferSize);
//     stagingBuffer.copy(buffer, bufferSize);
//     stagingBuffer.unmap();

//     createTextureImage(context, stagingBuffer, texWidth, texHeight);
//     createSampler();
// }

// TODO: This could come from a Descriptible interface (common w/ buffers)
vk::DescriptorImageInfo Texture::getDescriptor()
{
    return vk::DescriptorImageInfo{sampler.get(), view.get(), vk::ImageLayout::eShaderReadOnlyOptimal};
}

void Texture::createTextureImage(std::shared_ptr<core::Context> context, core::Buffer& buffer, uint32_t texWidth, uint32_t texHeight)
{
    // TODO: Move mipmaps generation out. Do we *need* it to happen before view creation ? We can also recreate the view
    // TODO: Can we change the "mipLevel" field of an image on the fly (to initialize it at 1 here)
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    initImage(texWidth, texHeight, mipLevels, vk::SampleCountFlagBits::e1,
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc);

    allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Transition the image to transfer dst layout
    context->device->commandpools.transfer.execute([&](vk::CommandBuffer cb) {
        transitionLayout(cb, vk::ImageLayout::eTransferDstOptimal);
        buffer.copyTo(cb, image.get(), texWidth, texHeight);
    });

    context->device->commandpools.graphics.execute([&](vk::CommandBuffer cb) {
        generateMipMaps(cb, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);
    });

    initView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}
} // namespace core