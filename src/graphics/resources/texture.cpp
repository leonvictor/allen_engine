#include "texture.hpp"
#include "../device.hpp"
#include "buffer.hpp"
#include "image.hpp"
#include <cmath>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace vkg
{
void Texture::CreateSampler(vk::SamplerAddressMode adressMode)
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

    m_sampler = m_pDevice->GetVkDevice().createSamplerUnique(samplerInfo);
}

Texture::Texture() {}

Texture::Texture(std::shared_ptr<Device> pDevice, std::string path)
{
    m_pDevice = pDevice;

    // Load image from file
    ImageFile img = ImageFile(path);

    // TODO: Move mipmaps generation out. Do we *need* it to happen before view creation ? We can also recreate the view
    // TODO: Can we change the "mipLevel" field of an image on the fly (to initialize it at 1 here)
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(img.width, img.height)))) + 1;

    InitImage(img.width, img.height, m_mipLevels, vk::SampleCountFlagBits::e1,
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc);

    Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Copy data to staging buffer
    Buffer stagingBuffer(m_pDevice, img.width * img.height * 4, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, img.pixels);
    m_pDevice->GetTransferCommandPool().Execute([&](vk::CommandBuffer cb)
                                                {
                                                    TransitionLayout(cb, vk::ImageLayout::eTransferDstOptimal);
                                                    // TODO: CopyFrom would be better here for example
                                                    stagingBuffer.CopyTo(cb, m_vkImage.get(), img.width, img.height);
                                                });

    m_pDevice->GetGraphicsCommandPool().Execute([&](vk::CommandBuffer cb)
                                                {
                                                    // TODO: this-> format ?
                                                    GenerateMipMaps(cb, vk::Format::eR8G8B8A8Srgb, img.width, img.height, m_mipLevels);
                                                });

    InitView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
    CreateSampler();
}

Texture::Texture(std::shared_ptr<Device> pDevice, vk::Image& image, vk::Format format, uint32_t mipLevels, vk::ImageAspectFlags aspectMask)
    : Image(pDevice, image, format, mipLevels, aspectMask)
{
    CreateSampler();
}

Texture::Texture(std::shared_ptr<Device> pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
                 vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask, vk::ImageLayout layout)
    : Image(pDevice, width, height, mipLevels, numSamples, format, tiling,
            usage, memProperties, aspectMask, layout)
{
    CreateSampler();
}

} // namespace vkg