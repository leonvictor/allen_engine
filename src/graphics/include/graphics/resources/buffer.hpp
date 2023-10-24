#pragma once

#include "allocation.hpp"

#include <vulkan/vulkan.hpp>

namespace aln
{
class RenderEngine;

namespace resources
{
class Image;

class Buffer : public Allocation
{
  private:
    vk::Buffer m_buffer;

  public:
    void Initialize(RenderEngine* pRenderEngine, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data = nullptr);

    template <typename T>
    void Initialize(RenderEngine* pRenderEngine, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const Vector<T>& data)
    {
        vk::DeviceSize size = sizeof(T) * data.size();
        Initialize(pRenderEngine, size, usage, memProperties, data.data());
    }

    void Shutdown() override;

    /// @brief Copy the content of this buffer to an image.
    void CopyTo(vk::CommandBuffer& cb, vk::Image& image, Vector<vk::BufferImageCopy> bufferCopyRegions) const;
    void CopyTo(vk::CommandBuffer& cb, vk::Image& image, const uint32_t width, const uint32_t height) const;
    void CopyTo(vk::CommandBuffer& cb, resources::Image& image) const;
    void CopyTo(vk::CommandBuffer& cb, Buffer& dstBuffer, const vk::DeviceSize& size) const;
    void CopyTo(vk::CommandBuffer& cb, Buffer& dstBuffer) const;

    inline vk::DescriptorBufferInfo GetDescriptor() const
    {
        return vk::DescriptorBufferInfo(m_buffer, 0, m_size);
    }

    inline const vk::Buffer& GetVkBuffer() const { return m_buffer; }
};
} // namespace resources
} // namespace aln