#pragma once


#include "resources/allocation.hpp"

#include "render_engine.hpp"

#include <utility>

namespace aln::resources
{
void Allocation::Allocate(const vk::MemoryRequirements& memRequirements, const vk::MemoryPropertyFlags& memProperties)
{
    vk::MemoryAllocateInfo allocInfo = {
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = m_pRenderEngine->FindMemoryType(memRequirements.memoryTypeBits, memProperties),
    };

    m_memory = m_pRenderEngine->GetVkDevice().allocateMemory(allocInfo, nullptr).value;
}

// Move assignement
Allocation& Allocation::operator=(Allocation&& other)
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
Allocation::Allocation(Allocation&& other)
{
    m_pRenderEngine = std::move(other.m_pRenderEngine);
    m_memory = std::move(other.m_memory);

    m_size = other.m_size;
    m_mapped = other.m_mapped;
}

void Allocation::Shutdown()
{
    m_pRenderEngine->GetVkDevice().freeMemory(m_memory);
}

void Allocation::Map(size_t offset, vk::DeviceSize size)
{
    assert(m_mapped == nullptr);
    m_mapped = m_pRenderEngine->GetVkDevice().mapMemory(m_memory, offset, size, vk::MemoryMapFlags()).value;
}

void Allocation::Unmap()
{
    assert(m_mapped != nullptr);
    m_pRenderEngine->GetVkDevice().unmapMemory(m_memory);
    m_mapped = nullptr;
}

void Allocation::Flush(vk::DeviceSize size, vk::DeviceSize offset)
{
    vk::MappedMemoryRange mappedRange = {
        .memory = m_memory,
        .offset = offset,
        .size = size,
    };

    m_pRenderEngine->GetVkDevice().flushMappedMemoryRanges(mappedRange);
}


void Allocation::Invalidate(vk::DeviceSize size, vk::DeviceSize offset)
{
    vk::MappedMemoryRange mappedRange = {
        .memory = m_memory,
        .offset = offset,
        .size = size,
    };

    m_pRenderEngine->GetVkDevice().invalidateMappedMemoryRanges(mappedRange);
}
}