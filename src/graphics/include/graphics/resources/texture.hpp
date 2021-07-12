#pragma once

#include "../device.hpp"
#include "buffer.hpp"
#include "image.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace vkg
{
// TODO: 2D and 3D
class Texture : public Image
{
  protected:
    void CreateSampler(vk::SamplerAddressMode adressMode = vk::SamplerAddressMode::eRepeat);
    vk::UniqueSampler m_sampler; // TODO: Does each image have a sampler or do they share it ?

  public:
    Texture();

    /// @brief Load a texture from a file.
    Texture(std::shared_ptr<Device> pDevice, std::string path);

    /// @brief Create a texture from a buffer
    /// @todo: use a Buffer object.
    Texture(void* buffer, vk::DeviceSize bufferSize, uint32_t texWidth, uint32_t texHeight);

    Texture(std::shared_ptr<Device> pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    Texture(std::shared_ptr<Device> pDevice, vk::Image& image, vk::Format format, uint32_t mipLevels, vk::ImageAspectFlags aspectMask);

    // TODO: This could come from a Descriptible interface (common w/ buffers)
    inline const vk::DescriptorImageInfo GetDescriptor() const
    {
        return vk::DescriptorImageInfo{m_sampler.get(), m_vkView.get(), m_layout};
    }

    vk::Sampler& GetVkSampler() { return m_sampler.get(); }

    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{
            {0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
        };

        return bindings;
    }

    vk::DescriptorSet& GetDescriptorSet()
    {
        // Lazy allocation of the descriptor set
        if (!m_descriptorSet)
        {
            m_descriptorSet = m_pDevice->AllocateDescriptorSet<Texture>();

            // Update the Descriptor Set:
            vk::WriteDescriptorSet writeDesc[1] = {};
            writeDesc[0].dstSet = m_descriptorSet.get();
            writeDesc[0].descriptorCount = 1;
            writeDesc[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            auto desc = GetDescriptor();
            writeDesc[0].pImageInfo = &desc;

            m_pDevice->GetVkDevice().updateDescriptorSets(1, writeDesc, 0, nullptr);
        }
        return m_descriptorSet.get();
    }
};
} // namespace vkg