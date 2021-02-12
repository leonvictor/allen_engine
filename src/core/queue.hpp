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

    Queue() {}
    operator vk::Queue() { return queue; }
};
} // namespace core