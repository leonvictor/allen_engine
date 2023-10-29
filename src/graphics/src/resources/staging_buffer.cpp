#include "resources/staging_buffer.hpp"

#include "render_engine.hpp"

namespace aln
{
void StagingBuffer::Initialize(RenderEngine* pRenderEngine, size_t size)
{
    m_pDevice = &pRenderEngine->GetVkDevice();

    m_buffer.Initialize(
        pRenderEngine,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    pRenderEngine->SetDebugUtilsObjectName(m_buffer.GetVkBuffer(), "Staging Buffer");

    m_mapping = m_buffer.Map<std::byte>();

    VmaVirtualBlockCreateInfo blockCreateInfo = {
        .size = size,
        .flags = VMA_VIRTUAL_BLOCK_CREATE_LINEAR_ALGORITHM_BIT,
    };

    vmaCreateVirtualBlock(&blockCreateInfo, &m_block);

    static constexpr vk::SemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
        .semaphoreType = vk::SemaphoreType::eTimeline,
        .initialValue = 0,
    };

    static constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo = {
        .pNext = &semaphoreTypeCreateInfo,
    };

    m_currentSemaphoreValue = 0;
    m_timelineSemaphore = m_pDevice->createSemaphore(semaphoreCreateInfo).value;
    pRenderEngine->SetDebugUtilsObjectName(m_timelineSemaphore, "Staging Buffer Upload Complete Semaphore");
}
} // namespace aln