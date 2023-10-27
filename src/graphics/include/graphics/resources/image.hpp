#pragma once

#include "allocation.hpp"

#include <common/maths/vec3.hpp>

#include <memory>
#include <stdexcept>
#include <string>

namespace aln
{
class GPUBuffer;

class GPUImage : public GPUAllocation
{
  protected:
    // Wrapped vulkan objects
    vk::Image m_image;
    vk::ImageView m_view;
    vk::Sampler m_sampler;
    vk::DescriptorSet m_descriptorSet;

    vk::ImageLayout m_layout;
    vk::Format m_format = vk::Format::eUndefined;
    uint32_t m_mipLevels = 1;
    uint32_t m_arrayLayers = 1;
    uint32_t m_width, m_height;

#ifdef ALN_DEBUG
    std::string m_debugName;
#endif

    // Set to true when the image resource is owned by an external agent (i.e. swapchain images)
    bool m_externallyOwnedImage = false;

    // Inner creation routines
    void InitImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, uint32_t arrayLayers = 1, vk::ImageCreateFlagBits flags = {}, vk::ImageLayout layout = vk::ImageLayout::eUndefined, vk::ImageType type = vk::ImageType::e2D);

    void InitView(vk::Format format, vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype = vk::ImageViewType::e2D);

  public:
    GPUImage() = default;

    GPUImage(const GPUImage&) = delete;
    GPUImage& operator=(const GPUImage&) = delete;
    
    GPUImage& operator=(GPUImage&& other)
    {
        if (this != &other)
        {
            GPUAllocation::operator=(std::move(other));

            m_sampler = std::move(other.m_sampler);
            m_view = std::move(other.m_view);
            m_image = std::move(other.m_image);
            m_descriptorSet = std::move(other.m_descriptorSet);

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

    GPUImage(GPUImage&& other) : GPUAllocation(std::move(other))
    {
        m_sampler = std::move(other.m_sampler);
        m_view = std::move(other.m_view);
        m_image = std::move(other.m_image);
        m_descriptorSet = std::move(other.m_descriptorSet);

        m_layout = other.m_layout;
        m_format = other.m_format;
        m_mipLevels = other.m_mipLevels;
        m_arrayLayers = other.m_arrayLayers;
        m_width = other.m_width;
        m_height = other.m_height;
        m_externallyOwnedImage = other.m_externallyOwnedImage;
    }

    /// @brief Create a new empty vulkan image.
    /// @todo Maybe move this to a Create() fn ?
    void Initialize(RenderEngine* pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
        vk::ImageUsageFlags usage, int arrayLayers = 1, vk::ImageCreateFlagBits flags = {}, vk::ImageLayout layout = vk::ImageLayout::eUndefined, vk::ImageType type = vk::ImageType::e2D);

    /// @todo Rename to make more explicit
    /// @brief Wrap an existing VkImage. The original image won't be automatically destroyed.
    void Initialize(RenderEngine* pDevice, vk::Image& image, vk::Format format);

    /// @brief Load a cubemap from a directory.
    //void InitializeFromCubemapFromDirectory(RenderEngine* pDevice, std::string path);

    void Shutdown() override;

    /// @brief Add a vulkan view to this image.
    void AddView(vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor, vk::ImageViewType viewtype = vk::ImageViewType::e2D);

    /// @brief Add a vulkan sampler to this image.
    void AddSampler(vk::SamplerAddressMode adressMode = vk::SamplerAddressMode::eRepeat);

    /// @brief [DEBUG ONLY] Set a debug name to this image and all its underlying vulkan objects.
    /// @todo Make sure this is a noop in release.
    void SetDebugName(std::string name);

    void Allocate(const vk::MemoryPropertyFlags& memProperties);

    void TransitionLayout(vk::CommandBuffer cb, vk::ImageLayout newLayout);

    void Blit(vk::CommandBuffer cb, GPUImage& dstImage);
    void Blit(vk::CommandBuffer cb, GPUImage& dstImage, uint32_t width, uint32_t height);

    // TODO: Would copyFrom methods be better ?
    // It's cool to keep data ownership
    void CopyTo(vk::CommandBuffer cb, GPUImage& dstImage);
    void CopyTo(vk::CommandBuffer cb, GPUImage& dstImage, uint32_t width, uint32_t height);

    /// @brief Generate mipmaps and transfer the last level to shader_readonly layout.
    void GenerateMipMaps(vk::CommandBuffer cb, uint32_t mipLevels);

    /// @brief Save image on disk as a ppm file.
    /// FIXME: This only works in 8-bits per channel formats
    void Save(std::string filename, bool colorSwizzle = false);

    /// @brief Retrieve the pixel value at index
    /// FIXME: This won't work if the image is in GPU-specific format
    /// FIXME: This only works in 8-bits per channel formats
    Vec3 PixelAt(uint32_t x, uint32_t y, bool colorSwizzle = false);

    // Accessors
    // TODO: Put const back when we've move to copyFrom functions
    vk::Image& GetVkImage() { return m_image; }
    const vk::ImageView& GetVkView() const { return m_view; }
    const vk::Sampler& GetVkSampler() const { return m_sampler; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    vk::ImageLayout GetLayout() const { return m_layout; }

    inline bool HasView() const { return (bool) m_view; }
    inline bool HasSampler() const { return (bool) m_sampler; }

    inline const vk::DescriptorImageInfo GetDescriptor() const
    {
        assert(m_view && m_sampler);
        return vk::DescriptorImageInfo{m_sampler, m_view, m_layout};
    }

    static Vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();

    const vk::DescriptorSet& GetDescriptorSet() const;
    void CreateDescriptorSet();
};
} // namespace aln::resources