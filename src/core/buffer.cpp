#include "buffer.hpp"
#include "allocation.hpp"
#include "image.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace core
{
Buffer::Buffer() {} // Empty ctor is required for now. Todo: Remove when we can

Buffer::Buffer(std::shared_ptr<core::Device> pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
{
    initialize(pDevice, size, usage, memProperties, data);
}

void Buffer::copyTo(vk::CommandBuffer& cb, vk::Image& image, std::vector<vk::BufferImageCopy> bufferCopyRegions) const
{
    cb.copyBufferToImage(buffer.get(), image, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
}

void Buffer::copyTo(vk::CommandBuffer& cb, core::Image& image) const
{
    copyTo(cb, image.image.get(), image.width, image.height);
}

void Buffer::copyTo(vk::CommandBuffer& cb, vk::Image& image, const uint32_t width, const uint32_t height) const
{
    vk::BufferImageCopy copy;
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageExtent = vk::Extent3D{width, height, 1};
    copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.mipLevel = 0;
    copy.imageOffset = vk::Offset3D{0, 0, 0};

    cb.copyBufferToImage(this->buffer.get(), image, vk::ImageLayout::eTransferDstOptimal, 1, &copy);
}

void Buffer::copyTo(vk::CommandBuffer& cb, core::Buffer& dstBuffer, const vk::DeviceSize& size) const
{
    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    cb.copyBuffer(this->buffer.get(), dstBuffer.buffer.get(), copyRegion);
}

void Buffer::copyTo(vk::CommandBuffer& cb, core::Buffer& dstBuffer) const
{
    // Default to dstBuffer's size
    copyTo(cb, dstBuffer, dstBuffer.size);
};

Buffer::operator vk::Buffer()
{
    return buffer.get();
}

void Buffer::createBuffer(const vk::DeviceSize& size, const vk::BufferUsageFlags& usage)
{
    // TODO: Move queues out of this function
    uint32_t queues[] = {m_pDevice->queueFamilyIndices.graphicsFamily.value(), m_pDevice->queueFamilyIndices.transferFamily.value()};

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    // TODO: We might have to pull this out as well
    bufferInfo.sharingMode = vk::SharingMode::eConcurrent; // Can buffers be shared between queues?
    bufferInfo.flags = vk::BufferCreateFlags();            // Configure sparse buffer memory. Not used rn

    bufferInfo.queueFamilyIndexCount = 2;
    bufferInfo.pQueueFamilyIndices = queues;

    buffer = m_pDevice->logical->createBufferUnique(bufferInfo);
}

void Buffer::allocate(const vk::MemoryPropertyFlags& memProperties)
{
    vk::MemoryRequirements memRequirements = m_pDevice->logical->getBufferMemoryRequirements(buffer.get());
    Allocation::allocate(memRequirements, memProperties);
    m_pDevice->logical->bindBufferMemory(buffer.get(), memory.get(), 0);
}

void Buffer::initialize(std::shared_ptr<core::Device> pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
{
    this->m_pDevice = pDevice;
    this->size = size;

    createBuffer(size, usage);
    allocate(memProperties);

    if (data != nullptr)
    {
        map(0, size);
        copy(data, size);

        // If host coherency hasn't been requested, do a manual flush to make writes visible
        // From samples
        if (!(memProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
        {
            vk::MappedMemoryRange mappedRange;
            mappedRange.memory = *memory;
            mappedRange.offset = 0;
            mappedRange.size = size;
            m_pDevice->logical->flushMappedMemoryRanges(mappedRange);
        }
        unmap();
    }
}
}; // namespace core