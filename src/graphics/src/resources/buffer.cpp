#include "resources/buffer.hpp"
#include "queue.hpp"
#include "resources/allocation.hpp"
#include "resources/image.hpp"
#include <memory>

#include <vulkan/vulkan.hpp>

namespace aln::vkg::resources
{
Buffer::Buffer() {} // Empty ctor is required for now. Todo: Remove when we can

Buffer::Buffer(Device* pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
{
    Initialize(pDevice, size, usage, memProperties, data);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vk::Image& vkImage, Vector<vk::BufferImageCopy> bufferCopyRegions) const
{
    cb.copyBufferToImage(m_vkBuffer.get(), vkImage, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions.size(), bufferCopyRegions.data());
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vkg::resources::Image& image) const
{
    CopyTo(cb, image.GetVkImage(), image.GetWidth(), image.GetHeight());
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vk::Image& vkImage, const uint32_t width, const uint32_t height) const
{
    vk::BufferImageCopy copy;
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.mipLevel = 0;
    copy.imageOffset = vk::Offset3D{0, 0, 0};
    copy.imageExtent = vk::Extent3D{
        .width = width,
        .height = height,
        .depth = 1,
    };
    cb.copyBufferToImage(m_vkBuffer.get(), vkImage, vk::ImageLayout::eTransferDstOptimal, 1, &copy);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, Buffer& dstBuffer, const vk::DeviceSize& size) const
{
    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    cb.copyBuffer(m_vkBuffer.get(), dstBuffer.m_vkBuffer.get(), copyRegion);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, Buffer& dstBuffer) const
{
    // Default to dstBuffer's size
    CopyTo(cb, dstBuffer, dstBuffer.m_size);
};

void Buffer::CreateBuffer(const vk::DeviceSize& size, const vk::BufferUsageFlags& usage)
{
    // TODO: Move queues out of this function
    uint32_t queues[] = {
        m_pDevice->GetGraphicsQueue().GetFamilyIndex(),
        m_pDevice->GetTransferQueue().GetFamilyIndex(),
    };

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    // TODO: We might have to pull this out as well
    bufferInfo.sharingMode = vk::SharingMode::eConcurrent; // Can buffers be shared between queues?
    bufferInfo.flags = vk::BufferCreateFlags();            // Configure sparse buffer memory. Not used rn

    bufferInfo.queueFamilyIndexCount = 2;
    bufferInfo.pQueueFamilyIndices = queues;

    m_vkBuffer = m_pDevice->GetVkDevice().createBufferUnique(bufferInfo).value;
}

void Buffer::Allocate(const vk::MemoryPropertyFlags& memProperties)
{
    auto memRequirements = m_pDevice->GetVkDevice().getBufferMemoryRequirements(m_vkBuffer.get());
    Allocation::Allocate(memRequirements, memProperties);
    m_pDevice->GetVkDevice().bindBufferMemory(m_vkBuffer.get(), m_memory.get(), 0);
}

void Buffer::Initialize(Device* pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
{
    m_pDevice = pDevice;
    m_size = size;

    CreateBuffer(m_size, usage);
    Allocate(memProperties);

    if (data != nullptr)
    {
        Map(0, m_size);
        Copy(data, m_size);

        // If host coherency hasn't been requested, do a manual flush to make writes visible
        // From samples
        if (!(memProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
        {
            vk::MappedMemoryRange mappedRange;
            mappedRange.memory = *m_memory;
            mappedRange.offset = 0;
            mappedRange.size = size;

            m_pDevice->GetVkDevice().flushMappedMemoryRanges(mappedRange);
        }
        Unmap();
    }
}
}; // namespace aln::vkg::resources