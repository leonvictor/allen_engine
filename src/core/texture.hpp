#pragma once

#include "../components.hpp"
#include "buffer.hpp"
#include "context.hpp"
#include "device.hpp"
#include "image.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace core
{
// TODO: 2D and 3D
class Texture : public core::Image, public Component
{
  protected:
    void createSampler(vk::SamplerAddressMode adressMode = vk::SamplerAddressMode::eRepeat);

  public:
    vk::UniqueSampler sampler; // TODO: Does each image have a sampler or do they share it ?

    Texture();

    // Load a texture from a file
    Texture(std::shared_ptr<core::Context> context, std::string path);

    // Load from a buffer
    Texture(std::shared_ptr<core::Context> context, void* buffer, vk::DeviceSize bufferSize, uint32_t texWidth, uint32_t texHeight);
    // TODO: This could come from a Descriptible interface (common w/ buffers)
    vk::DescriptorImageInfo getDescriptor();

  private:
    void createTextureImage(std::shared_ptr<core::Context> context, core::Buffer& buffer, uint32_t texWidth, uint32_t texHeight);

    // TODO :
    // - Move to image ? It's weird as long as we need context
    void generateMipMaps(vk::CommandBuffer& cb, vk::Format format, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);
};
} // namespace core