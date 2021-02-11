#pragma once

#include "allocation.hpp"
#include <vulkan/vulkan.hpp>

#include <memory>

namespace core
{
class Image;
class Device;

class Buffer : public Allocation
{
  public:
    vk::UniqueBuffer buffer;

    Buffer(); // Empty ctor is required for now. Todo: Remove when we can
    Buffer(std::shared_ptr<core::Device> device, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data = nullptr);

    template <typename T>
    Buffer(std::shared_ptr<core::Device> device, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const std::vector<T>& data)
    {
        vk::DeviceSize size = sizeof(T) * data.size();
        initialize(device, size, usage, memProperties, data.data());
    }

    void copyTo(vk::CommandBuffer& cb, vk::Image& image, std::vector<vk::BufferImageCopy> bufferCopyRegions) const;
    void copyTo(vk::CommandBuffer& cb, vk::Image& image, const uint32_t width, const uint32_t height) const;
    void copyTo(vk::CommandBuffer& cb, core::Image& image) const;
    void copyTo(vk::CommandBuffer& cb, core::Buffer& dstBuffer, const vk::DeviceSize& size) const;
    void copyTo(vk::CommandBuffer& cb, core::Buffer& dstBuffer) const;

    inline vk::DescriptorBufferInfo getDescriptor() const
    {
        return vk::DescriptorBufferInfo(buffer.get(), 0, size);
    }

    operator vk::Buffer();

  private:
    void createBuffer(const vk::DeviceSize& size, const vk::BufferUsageFlags& usage);
    void allocate(const vk::MemoryPropertyFlags& memProperties);
    void initialize(std::shared_ptr<core::Device>, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data);
};
} // namespace core