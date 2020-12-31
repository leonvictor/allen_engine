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
    vk::UniqueCommandPool pool;
    core::Queue queue;
    vk::Device device;

    CommandPool(){}; // TODO: We shouldn't need this
    CommandPool(vk::Device& device, core::Queue& queue, vk::CommandPoolCreateFlagBits flags = vk::CommandPoolCreateFlagBits());

    inline void execute(const std::function<void(vk::CommandBuffer cb)>& func)
    {
        vk::CommandBufferAllocateInfo cbai{this->pool.get(), vk::CommandBufferLevel::ePrimary, 1};
        auto cbs = device.allocateCommandBuffers(cbai);

        cbs[0].begin(vk::CommandBufferBeginInfo{});
        func(cbs[0]);
        cbs[0].end();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = (uint32_t) cbs.size();
        submitInfo.pCommandBuffers = cbs.data();

        queue.queue.submit(submitInfo, vk::Fence{});
        queue.queue.waitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete
        device.freeCommandBuffers(pool.get(), cbs);
    }

    std::vector<vk::CommandBuffer> beginSingleTimeCommands();
    void endSingleTimeCommands(std::vector<vk::CommandBuffer> commandBuffers);

    std::vector<vk::CommandBuffer> allocateCommandBuffers(int count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
    std::vector<vk::UniqueCommandBuffer> allocateCommandBuffersUnique(int count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
};
}; // namespace core