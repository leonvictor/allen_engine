#pragma once

#include <common/containers/vector.hpp>
#include <common/containers/vector_set.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{

class CommandPool;
class QueueSubmissionRequest;

struct TimelineSemaphore
{
    vk::Semaphore m_semaphore;
    uint64_t m_value; // Next value to signal
};

class ICommandBuffer
{
    template <typename T>
    friend class PersistentCommandPool;
    friend class QueueSubmissionRequest;
    friend class Queue;

    vk::CommandBuffer* m_pCommandBuffer = nullptr;

  protected:
    CommandPool* m_pSourceCommandPool = nullptr;
    ICommandBuffer() = default;
    ICommandBuffer(CommandPool* pCommandPool, vk::CommandBuffer* pCB) : m_pSourceCommandPool(pCommandPool), m_pCommandBuffer(pCB) {}

  public:
    explicit operator vk::CommandBuffer() { return *m_pCommandBuffer; }
    vk::CommandBuffer* operator->() { return m_pCommandBuffer; }
    operator bool() { return m_pCommandBuffer != nullptr; }
};

/// @note Those are non-owning handles
template <typename SyncPrimitive>
class PersistentCommandBuffer : public ICommandBuffer
{
    template <typename T>
    friend class PersistentCommandPool;
    friend class QueueSubmissionRequest;
    friend class Queue;

  private:
    SyncPrimitive* m_pSyncPrimitive = nullptr;

    PersistentCommandBuffer(CommandPool* pCommandPool, vk::CommandBuffer* pCB, SyncPrimitive* pSync) : ICommandBuffer(pCommandPool, pCB), m_pSyncPrimitive(pSync) {}

  public:
    PersistentCommandBuffer() = default;

    /// @brief Release the CB without submitting it
    void Release();
};

class TransientCommandBuffer : public ICommandBuffer
{
    friend class TransientCommandPool;
    friend class QueueSubmissionRequest;
    friend class Queue;

    TransientCommandBuffer(CommandPool* pCommandPool, vk::CommandBuffer* pCB) : ICommandBuffer(pCommandPool, pCB) {}

  public:
    TransientCommandBuffer() = default;
};

// --- Helper types
using TransferQueuePersistentCommandBuffer = PersistentCommandBuffer<TimelineSemaphore>;
using GraphicsQueuePersistentCommandBuffer = PersistentCommandBuffer<vk::Event>;

/// @brief Wraps a commands buffer with wait/signal semaphores that should be submtitted along
template <typename T>
class CommandBufferSubmission
{
    static_assert(std::is_base_of_v<ICommandBuffer, T>);

  private:
    T* m_pCommandBuffer = nullptr;

    Vector<const vk::Semaphore*> m_waitBinarySemaphores;
    Vector<vk::Semaphore*> m_signalBinarySemaphores;
    Vector<const vk::Semaphore*> m_waitTimelineSemaphores;
    Vector<vk::Semaphore*> m_signalTimelineSemaphores;
    Vector<uint64_t> m_waitTimelineSemaphoreValues;
    Vector<uint64_t> m_signalTimelineSemaphoreValues;

  public:
    T* GetCommandBuffer()
    {
        assert(m_pCommandBuffer != nullptr);
        return m_pCommandBuffer;
    }

    void Initialize(T* pCommandBuffer)
    {
        assert(m_pCommandBuffer == nullptr);
        assert(m_waitBinarySemaphores.empty());
        assert(m_signalBinarySemaphores.empty());
        assert(m_waitTimelineSemaphores.empty());
        assert(m_signalTimelineSemaphores.empty());
        assert(m_waitTimelineSemaphoreValues.empty());
        assert(m_signalTimelineSemaphoreValues.empty());

        m_pCommandBuffer = pCommandBuffer;
    }

    void Reset()
    {
        assert(m_pCommandBuffer != nullptr);

        m_pCommandBuffer = nullptr;

        m_waitBinarySemaphores.clear();
        m_signalBinarySemaphores.clear();
        m_waitTimelineSemaphores.clear();
        m_signalTimelineSemaphores.clear();
        m_waitTimelineSemaphoreValues.clear();
        m_signalTimelineSemaphoreValues.clear();
    }

    void WaitSemaphore(const vk::Semaphore* pSemaphore, uint64_t value)
    {
        assert(pSemaphore != nullptr);

        if (!VectorContains(m_waitTimelineSemaphores, pSemaphore))
        {
            m_waitTimelineSemaphores.push_back(pSemaphore);
            m_waitTimelineSemaphoreValues.push_back(value);
        }
#ifdef ALN_DEBUG
        else
        {
            auto it = VectorFind(m_waitTimelineSemaphores, pSemaphore);
            assert(m_waitTimelineSemaphoreValues[it - m_waitTimelineSemaphores.begin()] == value);
        }
#endif
    }

    void SignalSemaphore(vk::Semaphore* pSemaphore, uint64_t value)
    {
        if (!VectorContains(m_signalTimelineSemaphores, pSemaphore))
        {
            m_signalTimelineSemaphores.push_back(pSemaphore);
            m_signalTimelineSemaphoreValues.push_back(value);
        }
#ifdef ALN_DEBUG
        else
        {
            auto it = VectorFind(m_signalTimelineSemaphores, pSemaphore);
            assert(m_signalTimelineSemaphoreValues[it - m_signalTimelineSemaphores.begin()] == value);
        }
#endif
    }

    void PopulateRequest(QueueSubmissionRequest& request);
};

} // namespace aln