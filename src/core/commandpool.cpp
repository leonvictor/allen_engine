#pragma once

#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "queue.hpp"

namespace core
{
class CommandPool
{
  public:
    // TODO: refine handling of commandPools. Using RAII would be cool, but multiple classes hold references to the same pools (Swapchain images, picker, etc.)
    // shared_ptr ?
    vk::CommandPool pool;
    core::Queue queue;
    // std::shared_ptr<core::Device> device;
    vk::Device device;

    CommandPool() {} // TODO: We shouldn't need this

    CommandPool(vk::Device& device, core::Queue& queue, vk::CommandPoolCreateFlagBits flags = vk::CommandPoolCreateFlagBits())
    {
        this->device = device;
        this->queue = queue;

        vk::CommandPoolCreateInfo createInfo(flags, queue.family);
        pool = device.createCommandPool(createInfo);
    }

    operator vk::CommandPool() { return pool; }

    //TODO: Delete when we're done transfering to c++
    operator VkCommandPool() { return VkCommandPool(pool); }

    std::vector<vk::CommandBuffer> beginSingleTimeCommands()
    {
        assert(pool && "You are trying to use an unallocated command pool.");

        // TODO: Replace w/ allocate fn
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = pool;
        allocInfo.commandBufferCount = 1;

        auto commandBuffers = device.allocateCommandBuffers(allocInfo);

        // Immediately start recording
        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        for (int i = 0; i < commandBuffers.size(); i++)
        {
            commandBuffers[i].begin(beginInfo);
        }
        return commandBuffers;
    }

    inline void execute(const std::function<void(vk::CommandBuffer cb)>& func)
    {
        vk::CommandBufferAllocateInfo cbai{this->pool, vk::CommandBufferLevel::ePrimary, 1};
        auto cbs = device.allocateCommandBuffers(cbai);
        cbs[0].begin(vk::CommandBufferBeginInfo{});
        func(cbs[0]);
        cbs[0].end();
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = (uint32_t) cbs.size();
        submitInfo.pCommandBuffers = cbs.data();
        queue.queue.submit(submitInfo, vk::Fence{});
        queue.queue.waitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete
        device.freeCommandBuffers(pool, cbs);
    }

    // TODO: This would make more sense in a Queue class, which can decide how to handle the commands.
    void endSingleTimeCommands(std::vector<vk::CommandBuffer> commandBuffers)
    {
        assert(pool && "You are trying to use an unallocated command pool.");

        for (int i = 0; i < commandBuffers.size(); i++)
        {
            commandBuffers[i].end();
        }

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        submitInfo.pCommandBuffers = commandBuffers.data();

        queue.queue.submit(submitInfo, nullptr);
        queue.queue.waitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete

        device.freeCommandBuffers(pool, commandBuffers);
    }

    std::vector<vk::CommandBuffer> allocateCommandBuffers(int count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const
    {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = pool;
        allocInfo.commandBufferCount = count;
        allocInfo.level = level; // Or secondary

        return device.allocateCommandBuffers(allocInfo);
    }

  private:
};
}; // namespace core