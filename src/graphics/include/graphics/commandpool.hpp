#pragma once

#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "queue.hpp"

namespace aln::vkg
{
class CommandPool
{
  public:
    vk::UniqueCommandPool m_vkCommandPool;
    Queue* m_pQueue = nullptr;
    vk::Device* m_pVkDevice = nullptr;

    void Initialize(vk::Device* pDevice, Queue* pQueue, vk::CommandPoolCreateFlagBits flags);
    void Shutdown();

    /// @brief Execute some commands in a single-use command buffer.
    inline void Execute(const std::function<void(vk::CommandBuffer& cb)>& func)
    {
        vk::CommandBufferAllocateInfo cbai = {
            .commandPool = m_vkCommandPool.get(),
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };

        auto result = m_pVkDevice->allocateCommandBuffers(cbai);
        auto& cbs = result.value;

        cbs[0].begin(vk::CommandBufferBeginInfo{});
        func(cbs[0]);
        cbs[0].end();

        m_pQueue->Submit(cbs);
        m_pQueue->WaitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete
        m_pVkDevice->freeCommandBuffers(m_vkCommandPool.get(), cbs);
    }

    std::vector<vk::CommandBuffer> BeginSingleTimeCommands();
    void EndSingleTimeCommands(std::vector<vk::CommandBuffer> commandBuffers);

    std::vector<vk::CommandBuffer> AllocateCommandBuffers(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
    std::vector<vk::UniqueCommandBuffer> AllocateCommandBuffersUnique(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
};
} // namespace aln::vkg