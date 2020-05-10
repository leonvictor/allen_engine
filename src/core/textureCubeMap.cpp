#pragma once

#include <vulkan/vulkan.hpp>

#include "image.cpp"
#include "context.hpp"
#include "device.hpp"
#include "texture.cpp"

namespace core
{

class TextureCubeMap : public core::Texture
{ // TODO: inheritate a base Texture class
private:
public:
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

    void loadFromDirectory(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string path)
    {
        this->context = context;
        this->device = device;

        // TODO: Load the first face before the loop to initialize buffers
        auto facePath = "assets/skyboxes/daybreak/CloudyCrown_Daybreak_" + faces[0] + ".png";
        // TODO: Load face with stbi
        // Load image from file
        ImageFile img = ImageFile(facePath);

        vk::DeviceSize faceSize = img.width * img.height * img.channels;
        core::Buffer stagingBuffer = core::Buffer(device, faceSize * 6, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, faceSize);
        stagingBuffer.copy(img.pixels, static_cast<size_t>(faceSize));
        stagingBuffer.unmap();

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
            facePath = "assets/skyboxes/daybreak/CloudyCrown_Daybreak_" + faces[i] + ".png";
            img.load(facePath);
            // Copy data to staging buffer
            stagingBuffer.map(i * faceSize, faceSize);
            stagingBuffer.copy(img.pixels, static_cast<size_t>(faceSize));
            stagingBuffer.unmap();

            bufferImageCopy.bufferOffset = faceSize * i;
            bufferImageCopy.imageSubresource.mipLevel = 0; 
            bufferImageCopy.imageSubresource.baseArrayLayer = i;
            // bufferImageCopy.imageExtent = {img.width, img.height};
            bufferCopyRegions.push_back(bufferImageCopy);

        }

        initImage(img.width, img.height, 1,
                  vk::SampleCountFlagBits::e1,
                  vk::Format::eR8G8B8A8Srgb,
                  vk::ImageTiling::eOptimal,
                  vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
                  6,
                  vk::ImageCreateFlagBits::eCubeCompatible);

        initMemory(vk::MemoryPropertyFlagBits::eDeviceLocal);

        transitionLayout(context, vk::Format::eR8G8B8A8Srgb,
                         vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 6);

        context->copyBufferToImage(stagingBuffer, image, bufferCopyRegions);

        transitionLayout(context, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 6);
        
        initView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, 1, vk::ImageViewType::eCube, 6);

        createSampler(vk::SamplerAddressMode::eClampToEdge);
        stagingBuffer.destroy();
    };
};
} // namespace core