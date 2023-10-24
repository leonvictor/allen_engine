
#include "commandpool.hpp"
#include "queue.hpp"
#include "render_engine.hpp"

#include <vulkan/vulkan.hpp>

#include <functional>

namespace aln
{

void CommandPool::Reset()
{
    m_pVkDevice->resetCommandPool(m_vkCommandPool.get());
}

void CommandPool::AllocateCommandBuffers(uint32_t cbCount)
{
    assert(m_commandBuffers.empty()); // No growing allowed (for now)

    m_commandBuffers.resize(cbCount);

    vk::CommandBufferAllocateInfo allocInfo = {
        .commandPool = m_vkCommandPool.get(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = cbCount,
    };

    m_pVkDevice->allocateCommandBuffers(&allocInfo, m_commandBuffers.data());
}

void CommandPool::Initialize(vk::Device* pDevice, Queue* pQueue, vk::CommandPoolCreateFlagBits flags)
{
    m_pVkDevice = pDevice;
    m_pQueue = pQueue;

    vk::CommandPoolCreateInfo createInfo = {
        .flags = flags,
        .queueFamilyIndex = m_pQueue->GetFamilyIndex(),
    };

    m_vkCommandPool = m_pVkDevice->createCommandPoolUnique(createInfo).value;
}

void CommandPool::Shutdown()
{
    m_vkCommandPool.reset();
}

/// --- Event syncing

template <>
vk::Event PersistentCommandPool<vk::Event>::CreateSyncPrimitive() const
{
    return m_pVkDevice->createEvent({}).value;
}

template <>
bool PersistentCommandPool<vk::Event>::IsCommandBufferInUse(vk::Event* pEvent) const
{
    return m_pVkDevice->getEventStatus(*pEvent) == vk::Result::eEventSet;
}

template <>
void PersistentCommandPool<vk::Event>::SetCommandBufferInUse(vk::Event* pEvent)
{
    m_pVkDevice->setEvent(*pEvent);
}

template<>
void PersistentCommandPool<vk::Event>::SetCommandBufferAvailable(vk::Event* pEvent)
{
    m_pVkDevice->resetEvent(*pEvent);
}

// --- Semaphore syncing

template <>
TimelineSemaphore PersistentCommandPool<TimelineSemaphore>::CreateSyncPrimitive() const
{
    static constexpr vk::SemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
        .semaphoreType = vk::SemaphoreType::eTimeline,
        .initialValue = 0,
    };

    static constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo = {
        .pNext = &semaphoreTypeCreateInfo,
    };

    TimelineSemaphore timelineSemaphore = {
        .m_semaphore = m_pVkDevice->createSemaphore(semaphoreCreateInfo).value,
        .m_value = 0,
    };

    return timelineSemaphore;
}

template <>
bool PersistentCommandPool<TimelineSemaphore>::IsCommandBufferInUse(TimelineSemaphore* pSemaphore) const
{
    return m_pVkDevice->getSemaphoreCounterValue(pSemaphore->m_semaphore).value != pSemaphore->m_value;
}

template <>
void PersistentCommandPool<TimelineSemaphore>::SetCommandBufferInUse(TimelineSemaphore* pSemaphore)
{
    pSemaphore->m_value++;
}

template <>
void PersistentCommandPool<TimelineSemaphore>::SetCommandBufferAvailable(TimelineSemaphore* pSemaphore)
{
    pSemaphore->m_value--;
}

} // namespace aln