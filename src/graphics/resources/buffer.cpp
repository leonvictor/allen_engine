#include "buffer.hpp"
#include "allocation.hpp"
#include "image.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace vkg
{
Buffer::Buffer() {} // Empty ctor is required for now. Todo: Remove when we can

Buffer::Buffer(std::shared_ptr<core::Device> pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
{
    Initialize(pDevice, size, usage, memProperties, data);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vk::Image& vkImage, std::vector<vk::BufferImageCopy> bufferCopyRegions) const
{
    cb.copyBufferToImage(m_vkBuffer.get(), vkImage, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vkg::Image& image) const
{
    CopyTo(cb, image.GetVkImage(), image.width, image.height);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vk::Image& vkImage, const uint32_t width, const uint32_t height) const
{
    vk::BufferImageCopy copy{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageExtent = vk::Extent3D{width, height, 1},
        .imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        .imageSubresource.mipLevel = 0,
        .imageOffset = vk::Offset3D{0, 0, 0},
    };

    cb.copyBufferToImage(m_vkBuffer.get(), vkImage, vk::ImageLayout::eTransferDstOptimal, 1, &copy);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vkg::Buffer& dstBuffer, const vk::DeviceSize& size) const
{
    vk::BufferCopy copyRegion{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    cb.copyBuffer(m_vkBuffer.get(), dstBuffer.m_vkBuffer.get(), copyRegion);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vkg::Buffer& dstBuffer) const
{
    // Default to dstBuffer's size
    CopyTo(cb, dstBuffer, dstBuffer.m_size);
};

Buffer::operator vk::Buffer()
{
    return m_vkBuffer.get();
}

void Buffer::CreateBuffer(const vk::DeviceSize& size, const vk::BufferUsageFlags& usage)
{
    // TODO: Move queues out of this function
    uint32_t queues[] = {m_pDevice->queueFamilyIndices.graphicsFamily.value(), m_pDevice->queueFamilyIndices.transferFamily.value()};

    vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = usage,
        // TODO: We might have to pull this out as well
        .sharingMode = vk::SharingMode::eConcurrent, // Can buffers be shared between queues?
        .flags = vk::BufferCreateFlags(),            // Configure sparse buffer memory. Not used rn

        .queueFamilyIndexCount = 2,
        .pQueueFamilyIndices = queues,
    };

    m_buffer = m_pDevice->logical->createBufferUnique(bufferInfo);
}

void Buffer::Allocate(const vk::MemoryPropertyFlags& memProperties)
{
    vk::MemoryRequirements memRequirements = m_pDevice->logical->getBufferMemoryRequirements(m_vkBuffer.get());
    Allocation::Allocate(memRequirements, memProperties);
    m_pDevice->logical->bindBufferMemory(m_vkBuffer.get(), m_memory.get(), 0);
}

void Buffer::Initialize(std::shared_ptr<core::Device> pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
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
            vk::MappedMemoryRange mappedRange{
                .memory = *m_memory,
                .offset = 0,
                .size = size,
            };
            m_pDevice->logical->flushMappedMemoryRanges(mappedRange);
        }
        Unmap();
    }
}
}; // namespace vkg