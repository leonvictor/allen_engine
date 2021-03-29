#pragma once

#include "allocation.hpp"
#include <vulkan/vulkan.hpp>

#include <memory>

namespace vkg
{
class Image;
class Device;

class Buffer : public Allocation
{
  public:
    vk::UniqueBuffer m_vkBuffer;

    Buffer(); // Empty ctor is required for now. Todo: Remove when we can
    Buffer(std::shared_ptr<Device> device, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data = nullptr);

    template <typename T>
    Buffer(std::shared_ptr<Device> device, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const std::vector<T>& data)
    {
        vk::DeviceSize m_size = sizeof(T) * data.size();
        Initialize(device, m_size, usage, memProperties, data.data());
    }

    void CopyTo(vk::CommandBuffer& cb, vk::Image& image, std::vector<vk::BufferImageCopy> bufferCopyRegions) const;
    void CopyTo(vk::CommandBuffer& cb, vk::Image& image, const uint32_t width, const uint32_t height) const;
    void CopyTo(vk::CommandBuffer& cb, vkg::Image& image) const;
    void CopyTo(vk::CommandBuffer& cb, vkg::Buffer& dstBuffer, const vk::DeviceSize& size) const;
    void CopyTo(vk::CommandBuffer& cb, vkg::Buffer& dstBuffer) const;

    inline vk::DescriptorBufferInfo GetDescriptor() const
    {
        return vk::DescriptorBufferInfo(m_vkBuffer.get(), 0, m_size);
    }

    inline vk::Buffer& GetVkBuffer() { return m_vkBuffer.get(); }

  private:
    void CreateBuffer(const vk::DeviceSize& size, const vk::BufferUsageFlags& usage);
    void Allocate(const vk::MemoryPropertyFlags& memProperties);
    void Initialize(std::shared_ptr<Device> pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data);
};
} // namespace vkg