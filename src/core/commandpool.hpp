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
    vk::Device device;

    CommandPool(){}; // TODO: We shouldn't need this
    CommandPool(vk::Device& device, core::Queue& queue, vk::CommandPoolCreateFlagBits flags = vk::CommandPoolCreateFlagBits());

    operator vk::CommandPool() { return pool; }
    //TODO: Delete when we're done transfering to c++
    operator VkCommandPool() { return VkCommandPool(pool); }

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

    std::vector<vk::CommandBuffer> beginSingleTimeCommands();
    void endSingleTimeCommands(std::vector<vk::CommandBuffer> commandBuffers);

    std::vector<vk::CommandBuffer> allocateCommandBuffers(int count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
};
}; // namespace core