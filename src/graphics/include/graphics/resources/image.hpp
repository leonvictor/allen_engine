#pragma once

#include "allocation.hpp"

#include <memory>
#include <stdexcept>

#include <glm/vec3.hpp>
#include <stb_image.h>
#include <stdexcept>

namespace aln::vkg::resources
{
class Buffer;

class Image : public Allocation
{
  protected:
    // Wrapped vulkan objects
    vk::UniqueImage m_vkImage;
    vk::UniqueImageView m_vkView;
    vk::UniqueSampler m_vkSampler;
    vk::UniqueDescriptorSet m_vkDescriptorSet;

    vk::ImageLayout m_layout;
    vk::Format m_format = vk::Format::eUndefined;
    uint32_t m_mipLevels = 1;
    uint32_t m_arrayLayers = 1;
    uint32_t m_width, m_height;

#ifndef NDEBUG
    std::string m_debugName;
#endif

    // Set to true when the image resource is owned by an external agent (i.e. swapchain images)
    bool m_externallyOwnedImage = false;

    // Inner creation routines
    void InitImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, int arrayLayers = 1, vk::ImageCreateFlagBits flags = {}, vk::ImageLayout layout = vk::ImageLayout::eUndefined, vk::ImageType type = vk::ImageType::e2D);

    void InitView(vk::Format format, vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype = vk::ImageViewType::e2D);

  public:
    ~Image()
    {
        m_vkDescriptorSet.reset();
        m_vkSampler.reset();
        m_vkView.reset();

        if (m_externallyOwnedImage)
        {
            m_vkImage.release();
        }
        else
        {
            m_vkImage.reset();
        }
    }

    // No copy allowed
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    // Move assignement
    Image& operator=(Image&& other)
    {
        if (this != &other)
        {
            Allocation::operator=(std::move(other));

            m_vkSampler = std::move(other.m_vkSampler);
            m_vkView = std::move(other.m_vkView);
            m_vkImage = std::move(other.m_vkImage);
            m_vkDescriptorSet = std::move(other.m_vkDescriptorSet);

            m_layout = other.m_layout;
            m_format = other.m_format;
            m_mipLevels = other.m_mipLevels;
            m_arrayLayers = other.m_arrayLayers;
            m_width = other.m_width;
            m_height = other.m_height;
            m_externallyOwnedImage = other.m_externallyOwnedImage;
        }
        return *this;
    }

    // Move constructor
    Image(Image&& other) : Allocation(std::move(other))
    {
        m_vkSampler = std::move(other.m_vkSampler);
        m_vkView = std::move(other.m_vkView);
        m_vkImage = std::move(other.m_vkImage);
        m_vkDescriptorSet = std::move(other.m_vkDescriptorSet);

        m_layout = other.m_layout;
        m_format = other.m_format;
        m_mipLevels = other.m_mipLevels;
        m_arrayLayers = other.m_arrayLayers;
        m_width = other.m_width;
        m_height = other.m_height;
        m_externallyOwnedImage = other.m_externallyOwnedImage;
    }

    // Creation API
    Image() {}

    /// @brief Create a new empty vulkan image.
    /// @todo Maybe move this to a Create() fn ?
    Image(std::shared_ptr<Device> pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, int arrayLayers = 1, vk::ImageCreateFlagBits flags = {}, vk::ImageLayout layout = vk::ImageLayout::eUndefined, vk::ImageType type = vk::ImageType::e2D);

    /// @brief Wrap an existing VkImage. The original image won't be automatically destroyed.
    Image(std::shared_ptr<Device> pDevice, vk::Image& image, vk::Format format);

    /// @brief Create an image an upload the content of a buffer to it.
    static Image FromBuffer(std::shared_ptr<Device> pDevice, Buffer& buffer, uint32_t width, uint32_t height, uint32_t mipLevels = 1, uint32_t arrayLayers = 1, vk::ImageType type = vk::ImageType::e2D);

    /// @brief Load a cubemap from a directory.
    static Image CubemapFromDirectory(std::shared_ptr<Device> pDevice, std::string path);

    /// @brief Add a vulkan view to this image.
    void AddView(vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor, vk::ImageViewType viewtype = vk::ImageViewType::e2D);

    /// @brief Add a vulkan sampler to this image.
    void AddSampler(vk::SamplerAddressMode adressMode = vk::SamplerAddressMode::eRepeat);

    /// @brief [DEBUG ONLY] Set a debug name to this image and all its underlying vulkan objects.
    /// @todo Make sure this is a noop in release.
    void SetDebugName(std::string name)
    {
#ifndef NDEBUG
        m_debugName = name;
        if (m_vkImage)
        {
            m_pDevice->SetDebugUtilsObjectName(m_vkImage.get(), name + " Image");
        }
        if (m_vkView)
        {
            m_pDevice->SetDebugUtilsObjectName(m_vkView.get(), name + " Image View");
        }
        if (m_vkSampler)
        {
            m_pDevice->SetDebugUtilsObjectName(m_vkSampler.get(), name + " Sampler");
        }
        if (m_vkDescriptorSet)
        {
            m_pDevice->SetDebugUtilsObjectName(m_vkDescriptorSet.get(), name + " Descriptor Set");
        }
#endif
    }

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
} // namespace aln::vkg::resources