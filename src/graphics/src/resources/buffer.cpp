#include "resources/buffer.hpp"
#include "queue.hpp"
#include "resources/allocation.hpp"
#include "resources/image.hpp"
#include <memory>

#include <vulkan/vulkan.hpp>

namespace aln::resources
{
Buffer::Buffer() {} // Empty ctor is required for now. Todo: Remove when we can

Buffer::Buffer(RenderEngine* pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
{
    Initialize(pDevice, size, usage, memProperties, data);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vk::Image& vkImage, Vector<vk::BufferImageCopy> bufferCopyRegions) const
{
    cb.copyBufferToImage(m_vkBuffer.get(), vkImage, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions.size(), bufferCopyRegions.data());
}

void Buffer::CopyTo(vk::CommandBuffer& cb, resources::Image& image) const
{
    CopyTo(cb, image.GetVkImage(), image.GetWidth(), image.GetHeight());
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vk::Image& vkImage, const uint32_t width, const uint32_t height) const
{
    vk::BufferImageCopy copy = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1,
        },
    };
    cb.copyBufferToImage(m_vkBuffer.get(), vkImage, vk::ImageLayout::eTransferDstOptimal, 1, &copy);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, Buffer& dstBuffer, const vk::DeviceSize& size) const
{
    vk::BufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

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
        m_pRenderEngine->GetGraphicsQueue().GetFamilyIndex(),
        m_pRenderEngine->GetTransferQueue().GetFamilyIndex(),
    };

    vk::BufferCreateInfo bufferInfo = {
        .flags = {},
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eConcurrent, // Can buffers be shared between queues,
        .queueFamilyIndexCount = 2,
        .pQueueFamilyIndices = queues,
    };

    m_vkBuffer = m_pRenderEngine->GetVkDevice().createBufferUnique(bufferInfo).value;
}

void Buffer::Allocate(const vk::MemoryPropertyFlags& memProperties)
{
    auto memRequirements = m_pRenderEngine->GetVkDevice().getBufferMemoryRequirements(m_vkBuffer.get());
    Allocation::Allocate(memRequirements, memProperties);
    m_pRenderEngine->GetVkDevice().bindBufferMemory(m_vkBuffer.get(), m_memory.get(), 0);
}

void Buffer::Initialize(RenderEngine* pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
{
    m_pRenderEngine = pDevice;
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
            vk::MappedMemoryRange mappedRange = {
                .memory = *m_memory,
                .offset = 0,
                .size = size,
            };

            m_pRenderEngine->GetVkDevice().flushMappedMemoryRanges(mappedRange);
        }
        Unmap();
    }
}
}; // namespace aln::resources