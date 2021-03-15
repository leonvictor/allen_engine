#pragma once
#include <vulkan/vulkan.hpp>

namespace core
{
struct Queue
{
    // TODO: Find a better API (this leads to a lot of queue.queue 's or queues.graphics.queue)
    // Maybe a simple getter ?
    vk::Queue queue;
    uint32_t family;

    Queue(vk::Device& device, uint32_t family) : queue(device.getQueue(family, 0))
    {
        this->family = family;
    }

    void Submit(std::vector<vk::CommandBuffer> cbs)
    {
        vk::SubmitInfo submitInfo{
            .commandBufferCount = (uint32_t) cbs.size(),
            .pCommandBuffers = cbs.data(),
        };

        // TODO: use fences
        queue.submit(cbs, vk::Fence{});
    }

    Queue() {}
    operator vk::Queue() { return queue; }
};
} // namespace core