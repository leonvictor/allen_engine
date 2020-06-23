#pragma once

#include "allocation.hpp"
#include "device.hpp"
#include <vulkan/vulkan.hpp>

#include <memory>

namespace core
{
class Buffer : public Allocation
{
  public:
    vk::Buffer buffer;

    Buffer(); // Empty ctor is required for now. Todo: Remove when we can
    Buffer(std::shared_ptr<core::Device> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProperties);
    void destroy() override;

    void copyTo(vk::CommandBuffer& cb, vk::Image& image, std::vector<vk::BufferImageCopy> bufferCopyRegions) const;
    void copyTo(vk::CommandBuffer& cb, vk::Image& image, const uint32_t width, const uint32_t height) const;
    void copyTo(vk::CommandBuffer& cb, core::Buffer& dstBuffer, vk::DeviceSize size) const;

    inline vk::DescriptorBufferInfo getDescriptor() const
    {
        return vk::DescriptorBufferInfo(buffer, 0, size);
    }

    operator vk::Buffer();

  private:
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage);
    void allocate(const vk::MemoryPropertyFlags& memProperties);
};
} // namespace core