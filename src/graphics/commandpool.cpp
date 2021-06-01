#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "commandpool.hpp"
#include "queue.hpp"

namespace vkg
{
CommandPool::CommandPool(vk::Device* device, Queue* queue, vk::CommandPoolCreateFlagBits flags)
{
    m_pVkDevice = device;
    m_pQueue = queue;

    vk::CommandPoolCreateInfo createInfo(flags, m_pQueue->GetFamilyIndex());
    m_vkCommandPool = m_pVkDevice->createCommandPoolUnique(createInfo);
}

std::vector<vk::CommandBuffer> CommandPool::BeginSingleTimeCommands()
{
    assert(m_vkCommandPool && "You are trying to use an unallocated command pool.");

    auto commandBuffers = AllocateCommandBuffers(1);

    // Immediately start recording
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

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

std::vector<vk::CommandBuffer> CommandPool::AllocateCommandBuffers(int count, vk::CommandBufferLevel level) const
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = m_vkCommandPool.get();
    allocInfo.commandBufferCount = count;
    allocInfo.level = level; // Or secondary

    return m_pVkDevice->allocateCommandBuffers(allocInfo);
}

std::vector<vk::UniqueCommandBuffer> CommandPool::AllocateCommandBuffersUnique(int count, vk::CommandBufferLevel level) const
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = m_vkCommandPool.get();
    allocInfo.commandBufferCount = count;
    allocInfo.level = level; // Or secondary

    return m_pVkDevice->allocateCommandBuffersUnique(allocInfo);
}
}; // namespace vkg