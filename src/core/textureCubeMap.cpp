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
        "Back",
        "Down",
        "Front",
        "Left",
        "Right",
        "Up",
    };

    void loadFromDirectory(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string path)
    {

        // TODO: Load the first face before the loop to initialize buffers
        auto facePath = "/assets/skyboxes/daybreak/CloudCrown_Daybreak_" + faces[0] + ".png";
        // TODO: Load face with stbi
        // Load image from file
        ImageFile img = ImageFile(path);

        vk::DeviceSize faceSize = img.width * img.height * 4;
        core::Buffer stagingBuffer = core::Buffer(device, faceSize * 6, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, faceSize);
        stagingBuffer.copy(img.pixels, static_cast<size_t>(faceSize));
        stagingBuffer.unmap();

        for (int i = 1; i < faces.size(); i++)
        {
            facePath = "/assets/skyboxes/daybreak/CloudCrown_Daybreak_" + faces[i] + ".png";
            img.load(facePath);
            // Copy data to staging buffer
            stagingBuffer.map(i * faceSize, faceSize);
            stagingBuffer.copy(img.pixels, static_cast<size_t>(faceSize));
            stagingBuffer.unmap();
        }

        initImage(img.width, img.height, 1,
                  vk::SampleCountFlagBits::e1,
                  vk::Format::eR8G8B8A8Srgb,
                  vk::ImageTiling::eOptimal,
                  vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
                  6);

        initMemory(vk::MemoryPropertyFlagBits::eDeviceLocal);

        transitionLayout(context, vk::Format::eR8G8B8A8Srgb,
                         vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 6);

        context->copyBufferToImage(stagingBuffer, image, img.width, img.height);

        initView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, 1);

        createSampler();
        stagingBuffer.destroy();
    };
};
} // namespace core