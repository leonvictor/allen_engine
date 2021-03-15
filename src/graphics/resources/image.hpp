#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "allocation.hpp"
#include "context.hpp"
#include "device.hpp"

// #define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace core
{

// Minimal Cpp wrapper for STB loader
// TODO by order of preference :
// 1) Get rid of it
// 2) Find it a better name
// 3) Move it somewhere else
struct ImageFile
{
    stbi_uc* pixels;
    int width, height, channels;

    explicit ImageFile(std::string path);
    void load(std::string path);
};

class Image : public Allocation
{
  protected:
    void InitImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage, int arrayLayers = 1, vk::ImageCreateFlagBits flags = {}, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    void Allocate(const vk::MemoryPropertyFlags& memProperties);
    void InitView(vk::Format format, vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype = vk::ImageViewType::e2D);

    vk::UniqueImage m_vkImage;
    vk::UniqueImageView m_vkView;
    vk::ImageLayout m_layout;
    vk::Format m_format;
    uint32_t m_mipLevels;
    uint32_t m_arrayLayers;
    // TODO: Use vec2 ?
    uint32_t m_width, m_height;

  public:
    // Empty ctor to avoid errors. We should be able to get rid of it later on
    Image();

    // TODO: Default arguments
    Image(std::shared_ptr<core::Device> pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
          vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    /// @brief Create an image without a view.
    Image(std::shared_ptr<core::Device> pDevice, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
          vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    /// @brief Create an image from an existing vkImage
    Image(std::shared_ptr<core::Device> pDevice, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels, vk::ImageViewType viewtype = vk::ImageViewType::e2D, int layerCount = 1);

    operator vk::Image();

    void TransitionLayout(vk::CommandBuffer cb, vk::ImageLayout newLayout);

    void Blit(vk::CommandBuffer cb, vkg::Image& dstImage);
    void Blit(vk::CommandBuffer cb, vkg::Image& dstImage, int width, int height);

    // TODO: Would copyFrom methods be better ?
    // It's cool to keep data ownership
    void CopyTo(vk::CommandBuffer cb, vkg::Image& dstImage);
    void CopyTo(vk::CommandBuffer cb, vkg::Image& dstImage, int width, int height);
    void GenerateMipMaps(vk::CommandBuffer& cb, vk::Format format, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);
    // Save image on disk as a ppm file.
    // FIXME: This only works in 8-bits per channel formats
    void Save(std::string filename, bool colorSwizzle = false);

    /// @brief Retreive the pixel value at index
    /// FIXME: This won't work if the image is in GPU-specific format
    /// FIXME: This only works in 8-bits per channel formats
    glm::vec3 PixelAt(int x, int y, bool colorSwizzle = false);

    vk::Image& GetVkImage() { return m_vkImage; }
    vk::Imageview& GetVkView() { return m_vkView; }
};
} // namespace core