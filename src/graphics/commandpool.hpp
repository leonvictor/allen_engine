#pragma once

#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "queue.hpp"

namespace vkg
{
class CommandPool
{
  public:
    vk::UniqueCommandPool m_vkCommandPool;
    Queue* m_pQueue;
    // TODO: Use the wrapper ? Not possible as commandpools are mostly used inside the device wrapper
    vk::Device* m_pVkDevice;

    CommandPool(){}; // TODO: We shouldn't need this
    CommandPool(vk::Device* device, Queue* queue, vk::CommandPoolCreateFlagBits flags = vk::CommandPoolCreateFlagBits());

    /// @brief Execute some commands in a single-use command buffer.
    inline void Execute(const std::function<void(vk::CommandBuffer cb)>& func)
    {
        vk::CommandBufferAllocateInfo cbai;
        cbai.commandPool = m_vkCommandPool.get();
        cbai.level = vk::CommandBufferLevel::ePrimary;
        cbai.commandBufferCount = 1;

        auto cbs = m_pVkDevice->allocateCommandBuffers(cbai);

        cbs[0].begin(vk::CommandBufferBeginInfo{});
        func(cbs[0]);
        cbs[0].end();

        m_pQueue->Submit(cbs);
        m_pQueue->WaitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete
        m_pVkDevice->freeCommandBuffers(m_vkCommandPool.get(), cbs);
    }

    std::vector<vk::CommandBuffer> BeginSingleTimeCommands();
    void EndSingleTimeCommands(std::vector<vk::CommandBuffer> commandBuffers);

    std::vector<vk::CommandBuffer> AllocateCommandBuffers(int count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
    std::vector<vk::UniqueCommandBuffer> AllocateCommandBuffersUnique(int count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
};
} // namespace vkg