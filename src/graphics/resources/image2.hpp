#pragma once

#include "allocation.hpp"

#include <memory>
#include <stdexcept>

#include <glm/glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace vkg
{
class Buffer;

class Image : public Allocation
{
  protected:
    // Wrapped vulkan objects
    vk::UniqueImage m_vkImage;
    vk::UniqueImageView m_vkView;
    vk::UniqueSampler m_vkSampler;

    vk::ImageLayout m_layout;
    vk::Format m_format;
    uint32_t m_mipLevels;
    uint32_t m_arrayLayers;
    uint32_t m_width, m_height;

    vk::UniqueDescriptorSet m_descriptorSet;

    // Inner creation routines
    void InitImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, int arrayLayers = 1, vk::ImageCreateFlagBits flags = {}, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    void InitView(vk::Format format, vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype = vk::ImageViewType::e2D);

  public:
    // Creation API
    /// @brief Create a new empty vulkan image.
    /// @todo Maybe move this to a Create() fn ?
    Image(std::shared_ptr<Device> pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, int arrayLayers = 1, vk::ImageCreateFlagBits flags = {}, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    /// @brief Create an image an upload the content of a buffer to it.
    static Image FromBuffer(std::shared_ptr<Device> pDevice, Buffer& buffer, uint32_t width, uint32_t height, uint32_t mipLevels = 1, uint32_t arrayLayers = 1);

    /// @brief Load a texture asset from disk.
    static Image FromAsset(std::shared_ptr<Device>, std::string path);

    /// @brief Load a texture from an image. Prefer using assets if possible!
    static Image FromFile(std::shared_ptr<Device>, std::string path);

    /// @brief Load a cubemap from a directory.
    static Image CubemapFromDirectory(std::shared_ptr<Device> pDevice, std::string path);

    /// @brief Add a vulkan view to this image.
    void AddView(vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype = vk::ImageViewType::e2D);

    /// @brief Add a vulkan sampler to this image.
    void AddSampler(vk::SamplerAddressMode adressMode = vk::SamplerAddressMode::eRepeat);

    void Allocate(const vk::MemoryPropertyFlags& memProperties);

    void TransitionLayout(vk::CommandBuffer cb, vk::ImageLayout newLayout);

    void Blit(vk::CommandBuffer cb, Image& dstImage);
    void Blit(vk::CommandBuffer cb, Image& dstImage, int width, int height);

    // TODO: Would copyFrom methods be better ?
    // It's cool to keep data ownership
    void CopyTo(vk::CommandBuffer cb, Image& dstImage);
    void CopyTo(vk::CommandBuffer cb, Image& dstImage, int width, int height);

    /// @brief Generate mipmaps and transfer the last level to shader_readonly layout.
    void GenerateMipMaps(vk::CommandBuffer& cb, uint32_t mipLevels);

    /// @brief Save image on disk as a ppm file.
    /// FIXME: This only works in 8-bits per channel formats
    void Save(std::string filename, bool colorSwizzle = false);

    /// @brief Retrieve the pixel value at index
    /// FIXME: This won't work if the image is in GPU-specific format
    /// FIXME: This only works in 8-bits per channel formats
    glm::vec3 PixelAt(int x, int y, bool colorSwizzle = false);

    // Accessors
    // TODO: Put const back when we've move to copyFrom functions
    vk::Image& GetVkImage() { return m_vkImage.get(); }
    const vk::ImageView& GetVkView() const { return m_vkView.get(); }
    const vk::Sampler& GetVkSampler() const { return m_vkSampler.get(); }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    vk::ImageLayout GetLayout() const { return m_layout; }

    inline bool HasView() { return (bool) m_vkView; }
    inline bool HasSampler() { return (bool) m_vkSampler; }

    inline const vk::DescriptorImageInfo GetDescriptor() const
    {
        assert(m_vkView && m_vkSampler);
        return vk::DescriptorImageInfo{m_vkSampler.get(), m_vkView.get(), m_layout};
    }

    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();

    vk::DescriptorSet& GetDescriptorSet();
};
} // namespace vkg