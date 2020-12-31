#include "buffer.hpp"
#include "allocation.hpp"
#include "device.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace core
{
Buffer::Buffer() {} // Empty ctor is required for now. Todo: Remove when we can

Buffer::Buffer(std::shared_ptr<core::Device> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProperties, void* data)
{
    this->device = device;
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
            device->logical->flushMappedMemoryRanges(mappedRange);
        }
        unmap();
    }
}

void Buffer::copyTo(vk::CommandBuffer& cb, vk::Image& image, std::vector<vk::BufferImageCopy> bufferCopyRegions) const
{
    cb.copyBufferToImage(this->buffer.get(), image, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
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

void Buffer::copyTo(vk::CommandBuffer& cb, core::Buffer& dstBuffer, vk::DeviceSize size) const
{
    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    cb.copyBuffer(this->buffer.get(), dstBuffer.buffer.get(), copyRegion);
}

Buffer::operator vk::Buffer()
{
    return buffer.get();
}

void Buffer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage)
{
    // TODO: Move queues out of this function
    uint32_t queues[] = {device->queueFamilyIndices.graphicsFamily.value(), device->queueFamilyIndices.transferFamily.value()};

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    // TODO: We might have to pull this out as well
    bufferInfo.sharingMode = vk::SharingMode::eConcurrent; // Can buffers be shared between queues?
    bufferInfo.flags = vk::BufferCreateFlags();            // Configure sparse buffer memory. Not used rn

    bufferInfo.queueFamilyIndexCount = 2;
    bufferInfo.pQueueFamilyIndices = queues;

    buffer = device->logical.get().createBufferUnique(bufferInfo);
}

void Buffer::allocate(const vk::MemoryPropertyFlags& memProperties)
{
    vk::MemoryRequirements memRequirements = device->logical.get().getBufferMemoryRequirements(buffer.get());
    Allocation::allocate(memRequirements, memProperties);
    device->logical.get().bindBufferMemory(buffer.get(), memory.get(), 0);
}
}; // namespace core