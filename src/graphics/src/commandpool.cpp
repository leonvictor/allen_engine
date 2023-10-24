
#include "commandpool.hpp"
#include "queue.hpp"
#include "render_engine.hpp"

#include <vulkan/vulkan.hpp>

#include <functional>

namespace aln
{

void CommandPool::Reset()
{
    m_pLogicalDevice->resetCommandPool(m_commandPool);
}

void CommandPool::AllocateCommandBuffers(uint32_t cbCount)
{
    assert(m_commandBuffers.empty()); // No growing allowed (for now)

    m_commandBuffers.resize(cbCount);

    vk::CommandBufferAllocateInfo allocInfo = {
        .commandPool = m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = cbCount,
    };

    m_pLogicalDevice->allocateCommandBuffers(&allocInfo, m_commandBuffers.data());
}

void CommandPool::FreeCommandBuffers()
{
    m_pLogicalDevice->freeCommandBuffers(m_commandPool, m_commandBuffers);
    m_commandBuffers.clear();
}

void CommandPool::Initialize(vk::Device* pDevice, Queue* pQueue, vk::CommandPoolCreateFlagBits flags)
{
    m_pLogicalDevice = pDevice;
    m_pQueue = pQueue;

    vk::CommandPoolCreateInfo createInfo = {
        .flags = flags,
        .queueFamilyIndex = m_pQueue->GetFamilyIndex(),
    };

    m_commandPool = m_pLogicalDevice->createCommandPool(createInfo).value;
}

void CommandPool::Shutdown()
{
    m_pLogicalDevice->destroyCommandPool(m_commandPool);
}

/// --- Event syncing

template <>
vk::Event PersistentCommandPool<vk::Event>::CreateSyncPrimitive() const
{
    return m_pLogicalDevice->createEvent({}).value;
}

template <>
void PersistentCommandPool<vk::Event>::DestroySyncPrimitive(vk::Event* pEvent) const
{
    m_pLogicalDevice->destroyEvent(*pEvent);
}

template <>
bool PersistentCommandPool<vk::Event>::IsCommandBufferInUse(vk::Event* pEvent) const
{
    return m_pLogicalDevice->getEventStatus(*pEvent) == vk::Result::eEventSet;
}

template <>
void PersistentCommandPool<vk::Event>::SetCommandBufferInUse(vk::Event* pEvent)
{
    m_pLogicalDevice->setEvent(*pEvent);
}

template <>
void PersistentCommandPool<vk::Event>::SetCommandBufferAvailable(vk::Event* pEvent)
{
    m_pLogicalDevice->resetEvent(*pEvent);
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
        .m_semaphore = m_pLogicalDevice->createSemaphore(semaphoreCreateInfo).value,
        .m_value = 0,
    };

    return timelineSemaphore;
}

template <>
void PersistentCommandPool<TimelineSemaphore>::DestroySyncPrimitive(TimelineSemaphore* pSemaphore) const
{
    m_pLogicalDevice->destroySemaphore(pSemaphore->m_semaphore);
}

template <>
bool PersistentCommandPool<TimelineSemaphore>::IsCommandBufferInUse(TimelineSemaphore* pSemaphore) const
{
    return m_pLogicalDevice->getSemaphoreCounterValue(pSemaphore->m_semaphore).value != pSemaphore->m_value;
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