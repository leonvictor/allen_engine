#pragma once

#include "resources/allocation.hpp"

#include "render_engine.hpp"

#include <utility>

namespace aln
{
void GPUAllocation::Allocate(const vk::MemoryRequirements& memRequirements, const vk::MemoryPropertyFlags& memProperties)
{
    vk::MemoryAllocateInfo allocInfo = {
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = m_pRenderEngine->FindMemoryType(memRequirements.memoryTypeBits, memProperties),
    };

    m_memory = m_pRenderEngine->GetVkDevice().allocateMemory(allocInfo, nullptr).value;
}

// Move assignement
GPUAllocation& GPUAllocation::operator=(GPUAllocation&& other)
{
    if (this != &other)
    {
        m_pRenderEngine = std::move(other.m_pRenderEngine);
        m_memory = std::move(other.m_memory);

        m_size = other.m_size;
        m_mapped = other.m_mapped;
    }
    return *this;
}

// Move constructor
GPUAllocation::GPUAllocation(GPUAllocation&& other)
{
    m_pRenderEngine = std::move(other.m_pRenderEngine);
    m_memory = std::move(other.m_memory);

    m_size = other.m_size;
    m_mapped = other.m_mapped;
}

void GPUAllocation::Shutdown()
{
    m_pRenderEngine->GetVkDevice().freeMemory(m_memory);
}

void GPUAllocation::Map(size_t offset, vk::DeviceSize size)
{
    assert(m_mapped == nullptr);
    m_mapped = m_pRenderEngine->GetVkDevice().mapMemory(m_memory, offset, size, vk::MemoryMapFlags()).value;
}

void GPUAllocation::Unmap()
{
    assert(m_mapped != nullptr);
    m_pRenderEngine->GetVkDevice().unmapMemory(m_memory);
    m_mapped = nullptr;
}

void GPUAllocation::Flush(vk::DeviceSize size, vk::DeviceSize offset)
{
    vk::MappedMemoryRange mappedRange = {
        .memory = m_memory,
        .offset = offset,
        .size = size,
    };

    m_pRenderEngine->GetVkDevice().flushMappedMemoryRanges(mappedRange);
}

void GPUAllocation::Invalidate(vk::DeviceSize size, vk::DeviceSize offset)
{
    vk::MappedMemoryRange mappedRange = {
        .memory = m_memory,
        .offset = offset,
        .size = size,
    };

    m_pRenderEngine->GetVkDevice().invalidateMappedMemoryRanges(mappedRange);
}
} // namespace aln