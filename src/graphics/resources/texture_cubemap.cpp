#include <vulkan/vulkan.hpp>

#include "../device.hpp"
#include "image.hpp"
#include "texture.hpp"
#include "texture_cubemap.hpp"

namespace vkg
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

void TextureCubeMap::LoadFromDirectory(std::shared_ptr<Device> pDevice, std::string path)
{
    m_pDevice = pDevice;

    // TODO: Remove hardcoded path to textures
    // TODO: Load the first face before the loop to initialize buffers
    auto facePath = "assets/skyboxes/midday/CloudyCrown_Midday_" + faces[0] + ".png";
    // TODO: Load face with stbi
    // Load image from file
    ImageFile img = ImageFile(facePath);

    vk::DeviceSize faceSize = img.width * img.height * img.channels;
    Buffer stagingBuffer = Buffer(m_pDevice, faceSize * 6, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    stagingBuffer.Map(0, faceSize);
    stagingBuffer.Copy(img.pixels, static_cast<size_t>(faceSize));
    stagingBuffer.Unmap();

    std::vector<vk::BufferImageCopy> bufferCopyRegions;

    vk::BufferImageCopy bufferImageCopy;
    bufferImageCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    bufferImageCopy.imageSubresource.layerCount = 1;

    bufferImageCopy.imageExtent.depth = 1;
    bufferImageCopy.imageExtent.width = img.width;
    bufferImageCopy.imageExtent.height = img.height;

    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferCopyRegions.push_back(bufferImageCopy);

    for (uint32_t i = 1; i < faces.size(); i++)
    {
        facePath = "assets/skyboxes/midday/CloudyCrown_Midday_" + faces[i] + ".png";
        img.load(facePath);
        // Copy data to staging buffer
        stagingBuffer.Map(i * faceSize, faceSize);
        stagingBuffer.Copy(img.pixels, static_cast<size_t>(faceSize));
        stagingBuffer.Unmap();

        bufferImageCopy.bufferOffset = faceSize * i;
        bufferImageCopy.imageSubresource.mipLevel = 0;
        bufferImageCopy.imageSubresource.baseArrayLayer = i;
        // bufferImageCopy.imageExtent = {img.width, img.height};
        bufferCopyRegions.push_back(bufferImageCopy);
    }

    InitImage(img.width, img.height, 1,
              vk::SampleCountFlagBits::e1,
              vk::Format::eR8G8B8A8Srgb,
              vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
              6,
              vk::ImageCreateFlagBits::eCubeCompatible);

    Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);

    m_pDevice->GetTransferCommandPool().Execute([&](vk::CommandBuffer cb) {
        TransitionLayout(cb, vk::ImageLayout::eTransferDstOptimal);
        stagingBuffer.CopyTo(cb, m_vkImage.get(), bufferCopyRegions);
    });

    m_pDevice->GetGraphicsCommandPool().Execute([&](vk::CommandBuffer cb) {
        TransitionLayout(cb, vk::ImageLayout::eShaderReadOnlyOptimal);
    });

    InitView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube);

    CreateSampler(vk::SamplerAddressMode::eClampToEdge);
};
} // namespace vkg