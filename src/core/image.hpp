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

    ImageFile(std::string path);
    void load(std::string path);
};

class Image : public Allocation
{
  protected:
    void initImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage, int arrayLayers = 1, vk::ImageCreateFlagBits flags = {}, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    void allocate(const vk::MemoryPropertyFlags& memProperties);
    void initView(vk::Format format, vk::ImageAspectFlags aspectMask, vk::ImageViewType viewtype = vk::ImageViewType::e2D);

  public:
    vk::UniqueImage image;
    vk::UniqueImageView view;

    vk::ImageLayout layout;
    vk::Format format;
    int mipLevels;
    int arrayLayers;
    int width, height;

    // Empty ctor to avoid errors. We should be able to get rid of it later on
    Image();

    // TODO: Default arguments
    Image(std::shared_ptr<core::Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
          vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageAspectFlags aspectMask, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    // Create an image without a view.
    Image(std::shared_ptr<core::Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
          vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memProperties, vk::ImageLayout layout = vk::ImageLayout::eUndefined);

    operator vk::Image();

    void transitionLayout(vk::CommandBuffer cb, vk::ImageLayout newLayout);

    void blit(vk::CommandBuffer cb, core::Image& dstImage);
    void blit(vk::CommandBuffer cb, core::Image& dstImage, int width, int height);

    void copyTo(vk::CommandBuffer cb, core::Image& dstImage);

    void copyTo(vk::CommandBuffer cb, core::Image& dstImage, int width, int height);

    // Save image on disk as a ppm file.
    // FIXME: This only works in 8-bits per channel formats
    void save(std::string filename, bool colorSwizzle = false);

    // Retreive the pixel value at index
    // FIXME: This won't work if the image is in GPU-specific format
    // FIXME: This only works in 8-bits per channel formats
    glm::vec3 pixelAt(int x, int y, bool colorSwizzle = false);

    // Helper function to create image views
    // @note: TODO: Should this be somewere else ? It doesn't depend on image members at all and is called from other places.
    // If so what would be a good place ? Inside device ?
    //
    static vk::UniqueImageView createImageViewUnique(std::shared_ptr<core::Device> device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t mipLevels, vk::ImageViewType viewtype = vk::ImageViewType::e2D, int layerCount = 1);
};
} // namespace core