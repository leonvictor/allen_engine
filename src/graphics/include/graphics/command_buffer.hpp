#pragma once

#include <vulkan/vulkan.hpp>

namespace aln
{

class CommandPool;

struct TimelineSemaphore
{
    vk::Semaphore m_semaphore;
    uint64_t m_value; // Next value to signal
};

class ICommandBuffer
{
    template <typename T>
    friend class PersistentCommandPool;
    friend class Queue;

    vk::CommandBuffer* m_pCommandBuffer = nullptr;

  protected:
    CommandPool* m_pSourceCommandPool = nullptr;
    ICommandBuffer() = default;
    ICommandBuffer(CommandPool* pCommandPool, vk::CommandBuffer* pCB) : m_pSourceCommandPool(pCommandPool), m_pCommandBuffer(pCB) {}

  public:
    operator vk::CommandBuffer&() { return *m_pCommandBuffer; }
    vk::CommandBuffer* operator->() { return m_pCommandBuffer; }
    operator bool() { return m_pCommandBuffer != nullptr; }
};

/// @note Those are non-owning handles
template <typename SyncPrimitive>
class PersistentCommandBuffer : public ICommandBuffer
{
    template <typename T>
    friend class PersistentCommandPool;
    friend class Queue;

  private:
    SyncPrimitive* m_pSyncPrimitive = nullptr;

    PersistentCommandBuffer(CommandPool* pCommandPool, vk::CommandBuffer* pCB, SyncPrimitive* pSync) : ICommandBuffer(pCommandPool, pCB), m_pSyncPrimitive(pSync) {}

  public:
    PersistentCommandBuffer() = default;

    void Release();
};

class TransientCommandBuffer : public ICommandBuffer
{
    friend class TransientCommandPool;
    friend class Queue;

    TransientCommandBuffer(CommandPool* pCommandPool, vk::CommandBuffer* pCB) : ICommandBuffer(pCommandPool, pCB) {}

  public:
    TransientCommandBuffer() = default;
};

// --- Helper types
using TransferQueuePersistentCommandBuffer = PersistentCommandBuffer<TimelineSemaphore>;
using GraphicsQueuePersistentCommandBuffer = PersistentCommandBuffer<vk::Event>;

} // namespace aln