#pragma once

#include "command_buffer.hpp"
#include "queue.hpp"

#include <common/containers/vector.hpp>
#include <vulkan/vulkan.hpp>

#include <functional>

namespace aln
{

// TODO: Rename ICommandPool
class CommandPool
{
    friend class RenderEngine;

  private:
    vk::CommandPool m_commandPool;
    Vector<vk::CommandBuffer> m_commandBuffers; // CBs allocated from this pool

  protected:
    vk::Device* m_pLogicalDevice = nullptr;
    Queue* m_pQueue = nullptr;

    void Initialize(vk::Device* pDevice, Queue* pQueue, vk::CommandPoolCreateFlagBits flags);
    void Shutdown();

    void Reset();

    vk::CommandBuffer* GetCommandBuffer(uint32_t commandBufferIdx)
    {
        assert(commandBufferIdx < m_commandBuffers.size());
        return &m_commandBuffers[commandBufferIdx];
    }

    void AllocateCommandBuffers(uint32_t commandBufferCount);
    void FreeCommandBuffers();

    uint32_t GetCacheSize() const { return m_commandBuffers.size(); }
};

/// @brief Transient command pools are reset every frame. The CBs they allocate are thus only usable during frameQueueCount frames.
/// They are however cheaper to use so should be chosen whenever possible !
class TransientCommandPool : public CommandPool
{
    uint32_t m_nextAvailableBufferIdx = 0;

  public:
    void Initialize(vk::Device* pDevice, Queue* pQueue)
    {
        CommandPool::Initialize(pDevice, pQueue, vk::CommandPoolCreateFlagBits::eTransient);
        AllocateCommandBuffers(5); // TODO: how many ?
    }

    void Shutdown()
    {
        FreeCommandBuffers();
        CommandPool::Shutdown();
    }

    void Reset()
    {
        CommandPool::Reset();
        m_nextAvailableBufferIdx = 0;
    }

    TransientCommandBuffer GetCommandBuffer()
    {
        // TODO: Growing strategy ?
        assert(m_nextAvailableBufferIdx < GetCacheSize() - 1);

        auto pCB = CommandPool::GetCommandBuffer(m_nextAvailableBufferIdx);
        m_nextAvailableBufferIdx++;

        vk::CommandBufferBeginInfo commandBufferBeginInfo = {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
        pCB->begin(&commandBufferBeginInfo);

        return TransientCommandBuffer(this, pCB);
    }
};

/// @brief Persistent command pools are never reset, and instead reset individual CBs after they finish executing.
/// Those CBs are thus available across frame boundaries, but are more costly to use
template <typename SyncPrimitive>
class PersistentCommandPool : public CommandPool
{
    uint32_t m_nextAvailableBufferIdx = 0;
    Vector<SyncPrimitive> m_syncPrimitives;

  private:
    SyncPrimitive CreateSyncPrimitive() const;
    void DestroySyncPrimitive(SyncPrimitive* pSyncPrimitive) const;

    bool IsCommandBufferInUse(SyncPrimitive* pSyncPrimitive) const;
    void SetCommandBufferInUse(SyncPrimitive* pSyncPrimitive);
    void SetCommandBufferAvailable(SyncPrimitive* pSyncPrimitive);

    void AllocateCommandBuffers(uint32_t commandBufferCount)
    {
        CommandPool::AllocateCommandBuffers(commandBufferCount);
        
        m_syncPrimitives.resize(commandBufferCount);
        for (auto syncIdx = 0; syncIdx < commandBufferCount; ++syncIdx)
        {
            m_syncPrimitives[syncIdx] = CreateSyncPrimitive();
        }
    }

    void FreeCommandBuffers()
    {
        for (auto& syncPrimitive : m_syncPrimitives)
        {
            DestroySyncPrimitive(&syncPrimitive);
        }
        m_syncPrimitives.clear();

        CommandPool::FreeCommandBuffers();
    }

  public:
    void Initialize(vk::Device* pDevice, Queue* pQueue)
    {
        CommandPool::Initialize(pDevice, pQueue, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        AllocateCommandBuffers(5); // TODO: how many ?
    }

    void Shutdown()
    {
        FreeCommandBuffers();
        CommandPool::Shutdown();
    }

    PersistentCommandBuffer<SyncPrimitive> GetCommandBuffer()
    {
        const auto startIdx = m_nextAvailableBufferIdx;
        bool availableCBFound = false;

        SyncPrimitive* pSyncPrimitive = nullptr;
        vk::CommandBuffer* pCB = nullptr;

        while (!availableCBFound)
        {
            pSyncPrimitive = &m_syncPrimitives[m_nextAvailableBufferIdx];
            pCB = CommandPool::GetCommandBuffer(m_nextAvailableBufferIdx);

            if (!IsCommandBufferInUse(pSyncPrimitive))
            {
                pCB->reset();
                SetCommandBufferInUse(pSyncPrimitive);

                availableCBFound = true;
            }
            m_nextAvailableBufferIdx = (m_nextAvailableBufferIdx + 1) % GetCacheSize();
            assert(m_nextAvailableBufferIdx != startIdx); // We looped through the buffer without finding a proper candidate. No growing for now !
        }

        vk::CommandBufferBeginInfo commandBufferBeginInfo = {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
        pCB->begin(&commandBufferBeginInfo);

        return PersistentCommandBuffer<SyncPrimitive>(this, pCB, pSyncPrimitive);
    }

    void ReleaseCommandBuffer(PersistentCommandBuffer<SyncPrimitive>& commandBuffer)
    {
        commandBuffer.m_pCommandBuffer->end();

        SetCommandBufferAvailable(commandBuffer.m_pSyncPrimitive);
        commandBuffer.m_pSyncPrimitive = nullptr;
        commandBuffer.m_pCommandBuffer = nullptr;
    }

    void Execute(const std::function<void(vk::CommandBuffer& cb)>& func)
    {
        auto cb = GetCommandBuffer();

        func(cb);

        Queue::SubmissionRequest request;
        request.ExecuteCommandBuffer(cb);
        m_pQueue->Submit(request, vk::Fence{});
    }
};

// --- Helper types
using TransferQueuePersistentCommandPool = PersistentCommandPool<TimelineSemaphore>;
using GraphicsQueuePersistentCommandPool = PersistentCommandPool<vk::Event>;

} // namespace aln