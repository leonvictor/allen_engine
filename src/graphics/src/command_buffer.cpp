#include "command_buffer.hpp"

#include "commandpool.hpp"
#include "queue.hpp"

namespace aln
{
template <typename SyncPrimitive>
void PersistentCommandBuffer<SyncPrimitive>::Release()
{
    auto pSourcePersistentPool = reinterpret_cast<PersistentCommandPool<SyncPrimitive>*>(m_pSourceCommandPool);
    pSourcePersistentPool->ReleaseCommandBuffer(*this);
}

template class PersistentCommandBuffer<vk::Event>;
template class PersistentCommandBuffer<TimelineSemaphore>;

template <typename T>
void CommandBufferSubmission<T>::PopulateRequest(QueueSubmissionRequest& request)
{
    assert(m_signalTimelineSemaphores.size() == m_signalTimelineSemaphoreValues.size());
    assert(m_waitTimelineSemaphores.size() == m_waitTimelineSemaphoreValues.size());
    

    auto signalTimelineSemaphoreCount = m_signalTimelineSemaphores.size();
    for (auto semaphoreIdx = 0; semaphoreIdx < signalTimelineSemaphoreCount; ++semaphoreIdx)
    {
        request.SignalSemaphore(*m_signalTimelineSemaphores[semaphoreIdx], m_signalTimelineSemaphoreValues[semaphoreIdx]);
    }

    auto waitTimelineSemaphoreCount = m_waitTimelineSemaphores.size();
    for (auto semaphoreIdx = 0; semaphoreIdx < waitTimelineSemaphoreCount; ++semaphoreIdx)
    {
        request.WaitSemaphore(*m_waitTimelineSemaphores[semaphoreIdx], m_waitTimelineSemaphoreValues[semaphoreIdx]);
    }

    for (auto& pSignalBinarySemaphore : m_signalBinarySemaphores)
    {
        request.SignalSemaphore(*pSignalBinarySemaphore);
    }

    for (auto& pWaitBinarySemaphore : m_waitBinarySemaphores)
    {
        request.WaitSemaphore(*pWaitBinarySemaphore);
    }

    request.ExecuteCommandBuffer(*m_pCommandBuffer);
}

template class CommandBufferSubmission<PersistentCommandBuffer<vk::Event>>;
template class CommandBufferSubmission<PersistentCommandBuffer<TimelineSemaphore>>;

} // namespace aln
