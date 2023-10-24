#include "resources/buffer.hpp"
#include "queue.hpp"
#include "render_engine.hpp"
#include "resources/allocation.hpp"
#include "resources/image.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace aln::resources
{
void Buffer::Initialize(RenderEngine* pDevice, const vk::DeviceSize& size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& memProperties, const void* data)
{
    m_pRenderEngine = pDevice;
    m_size = size;

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

    m_buffer = m_pRenderEngine->GetVkDevice().createBuffer(bufferInfo).value;

    auto memRequirements = m_pRenderEngine->GetVkDevice().getBufferMemoryRequirements(m_buffer);
    Allocation::Allocate(memRequirements, memProperties);

    m_pRenderEngine->GetVkDevice().bindBufferMemory(m_buffer, m_memory, 0);

    if (data != nullptr)
    {
        Map(0, m_size);
        Copy(data, m_size);

        // If host coherency hasn't been requested, do a manual flush to make writes visible
        if (!(memProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
        {
            vk::MappedMemoryRange mappedRange = {
                .memory = m_memory,
                .offset = 0,
                .size = size,
            };

            m_pRenderEngine->GetVkDevice().flushMappedMemoryRanges(mappedRange);
        }
        Unmap();
    }
}

void Buffer::Shutdown()
{
    m_pRenderEngine->GetVkDevice().destroyBuffer(m_buffer);
    Allocation::Shutdown();
}

void Buffer::CopyTo(vk::CommandBuffer& cb, vk::Image& vkImage, Vector<vk::BufferImageCopy> bufferCopyRegions) const
{
    cb.copyBufferToImage(m_buffer, vkImage, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions.size(), bufferCopyRegions.data());
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
    cb.copyBufferToImage(m_buffer, vkImage, vk::ImageLayout::eTransferDstOptimal, 1, &copy);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, Buffer& dstBuffer, const vk::DeviceSize& size) const
{
    vk::BufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    cb.copyBuffer(m_buffer, dstBuffer.m_buffer, copyRegion);
}

void Buffer::CopyTo(vk::CommandBuffer& cb, Buffer& dstBuffer) const
{
    // Default to dstBuffer's size
    CopyTo(cb, dstBuffer, dstBuffer.m_size);
};
}; // namespace aln::resources