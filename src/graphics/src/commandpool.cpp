
#include "commandpool.hpp"
#include "queue.hpp"
#include "render_engine.hpp"

#include <vulkan/vulkan.hpp>

#include <functional>

namespace aln
{

void CommandPool::Reset()
{
    m_freeCommandBufferIdx = 0;
    m_pVkDevice->resetCommandPool(m_vkCommandPool.get());
}

vk::CommandBuffer* CommandPool::RequestCommandBuffer()
{
    assert(m_freeCommandBufferIdx < MAX_CACHED_COMMAND_BUFFERS);

    if (m_freeCommandBufferIdx > m_allocatedCommandBuffers.size() - 1)
    {
        assert(false);
        // TODO: Grow the cached buffer pool
    }

    auto pCB = &m_allocatedCommandBuffers[m_freeCommandBufferIdx];
    m_freeCommandBufferIdx++;
    return pCB;
}

void CommandPool::AllocateCommandBuffers(std::span<vk::CommandBuffer> commandBuffers)
{
    vk::CommandBufferAllocateInfo allocInfo = {
        .commandPool = m_vkCommandPool.get(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
    };

    m_pVkDevice->allocateCommandBuffers(&allocInfo, commandBuffers.data());
}

void CommandPool::AllocateCommandBuffers(vk::CommandBuffer* pOut)
{
    vk::CommandBufferAllocateInfo allocInfo = {
        .commandPool = m_vkCommandPool.get(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };

    m_pVkDevice->allocateCommandBuffers(&allocInfo, pOut);
}

vk::UniqueCommandBuffer CommandPool::AllocateUniqueCommandBuffer()
{
    vk::CommandBufferAllocateInfo allocInfo = {
        .commandPool = m_vkCommandPool.get(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };

    return std::move(m_pVkDevice->allocateCommandBuffersUnique(allocInfo).value[0]);
}

void CommandPool::FreeCommandBuffers(vk::CommandBuffer* pBuffer, uint32_t bufferCount)
{
    m_pVkDevice->freeCommandBuffers(m_vkCommandPool.get(), bufferCount, pBuffer);
}

void CommandPool::Initialize(vk::Device* pDevice, Queue* pQueue, vk::CommandPoolCreateFlagBits flags, uint32_t cachedCommandBuffersCount)
{
    m_pVkDevice = pDevice;
    m_pQueue = pQueue;

    vk::CommandPoolCreateInfo createInfo{
        .flags = flags,
        .queueFamilyIndex = m_pQueue->GetFamilyIndex(),
    };

    m_vkCommandPool = m_pVkDevice->createCommandPoolUnique(createInfo).value;

    if (cachedCommandBuffersCount > 0)
    {
        m_allocatedCommandBuffers.resize(cachedCommandBuffersCount);
        AllocateCommandBuffers(m_allocatedCommandBuffers);
    }
}

void CommandPool::Shutdown()
{
    m_vkCommandPool.reset();
}

std::vector<vk::CommandBuffer> CommandPool::BeginSingleTimeCommands()
{
    assert(m_vkCommandPool && "You are trying to use an unallocated command pool.");

    auto commandBuffers = AllocateCommandBuffers(1);

    // Immediately start recording
    vk::CommandBufferBeginInfo beginInfo = {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };

    for (vk::CommandBuffer cb : commandBuffers)
    {
        cb.begin(beginInfo);
    }
    return commandBuffers;
}

// TODO: This would make more sense in a Queue class, which can decide how to handle the commands.
// TODO: Replace by the new API. Might still see some use (with modification for finer control over the schedule)
void CommandPool::EndSingleTimeCommands(std::vector<vk::CommandBuffer> commandBuffers)
{
    assert(m_vkCommandPool && "You are trying to use an unallocated command pool.");

    for (vk::CommandBuffer cb : commandBuffers)
    {
        cb.end();
    }

    m_pQueue->Submit(commandBuffers);
    m_pQueue->WaitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete

    m_pVkDevice->freeCommandBuffers(m_vkCommandPool.get(), commandBuffers);
}

std::vector<vk::CommandBuffer> CommandPool::AllocateCommandBuffers(uint32_t count, vk::CommandBufferLevel level) const
{
    vk::CommandBufferAllocateInfo allocInfo = {
        .commandPool = m_vkCommandPool.get(),
        .level = level,
        .commandBufferCount = count,
    };

    return m_pVkDevice->allocateCommandBuffers(allocInfo).value;
}

std::vector<vk::UniqueCommandBuffer> CommandPool::AllocateCommandBuffersUnique(uint32_t count, vk::CommandBufferLevel level) const
{
    vk::CommandBufferAllocateInfo allocInfo = {
        .commandPool = m_vkCommandPool.get(),
        .level = level,
        .commandBufferCount = count,
    };

    auto result = m_pVkDevice->allocateCommandBuffersUnique(allocInfo);
    return std::move(result.value);
}
}; // namespace aln