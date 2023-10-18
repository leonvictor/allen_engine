#pragma once

#include "queue.hpp"

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

#include <functional>

namespace aln
{
class CommandPool
{
    friend class RenderEngine;

    static constexpr uint32_t MAX_CACHED_COMMAND_BUFFERS = 5;
  
private:
    vk::UniqueCommandPool m_vkCommandPool;
    Queue* m_pQueue = nullptr;
    vk::Device* m_pVkDevice = nullptr;
    Vector<vk::CommandBuffer> m_allocatedCommandBuffers;

    uint32_t m_freeCommandBufferIdx = 0;

  public:
    /// @brief Initialize a command buffer pool
    /// @param cachedCommandBuffersCount number of buffers to pre-allocate 
    void Initialize(vk::Device* pDevice, Queue* pQueue, vk::CommandPoolCreateFlagBits flags, uint32_t cachedCommandBuffersCount = 0);
    void Shutdown();

    void Reset();

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

    vk::CommandBuffer* RequestCommandBuffer();

    void AllocateCommandBuffers(std::span<vk::CommandBuffer> commandBuffers);
    void AllocateCommandBuffers(vk::CommandBuffer* pOut);
    vk::UniqueCommandBuffer AllocateUniqueCommandBuffer();

    void FreeCommandBuffers(vk::CommandBuffer* pBuffer, uint32_t bufferCount = 1);

    std::vector<vk::CommandBuffer> AllocateCommandBuffers(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
    std::vector<vk::UniqueCommandBuffer> AllocateCommandBuffersUnique(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
};
} // namespace aln