#include "command_buffer.hpp"

#include "commandpool.hpp"

namespace aln
{
template<typename SyncPrimitive>
void PersistentCommandBuffer<SyncPrimitive>::Release()
{
    auto pSourcePersistentPool = reinterpret_cast<PersistentCommandPool<SyncPrimitive>*>(m_pSourceCommandPool);
    pSourcePersistentPool->ReleaseCommandBuffer(*this);
}

template class PersistentCommandBuffer<vk::Event>;
template class PersistentCommandBuffer<TimelineSemaphore>;
}
