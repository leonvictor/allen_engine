#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "commandpool.hpp"
#include "queue.hpp"

namespace core
{
CommandPool::CommandPool(vk::Device& device, core::Queue& queue, vk::CommandPoolCreateFlagBits flags)
{
    this->device = device;
    this->queue = queue;

    vk::CommandPoolCreateInfo createInfo(flags, queue.family);
    pool = device.createCommandPoolUnique(createInfo);
}

std::vector<vk::CommandBuffer> CommandPool::beginSingleTimeCommands()
{
    assert(pool && "You are trying to use an unallocated command pool.");

    auto commandBuffers = allocateCommandBuffers(1);

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
void CommandPool::endSingleTimeCommands(std::vector<vk::CommandBuffer> commandBuffers)
{
    assert(pool && "You are trying to use an unallocated command pool.");

    for (vk::CommandBuffer cb : commandBuffers)
    {
        cb.end();
    }

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();

    queue.queue.submit(submitInfo, nullptr);
    queue.queue.waitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete

    device.freeCommandBuffers(pool.get(), commandBuffers);
}

std::vector<vk::CommandBuffer> CommandPool::allocateCommandBuffers(int count, vk::CommandBufferLevel level) const
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = pool.get();
    allocInfo.commandBufferCount = count;
    allocInfo.level = level; // Or secondary

    return device.allocateCommandBuffers(allocInfo);
}
}; // namespace core