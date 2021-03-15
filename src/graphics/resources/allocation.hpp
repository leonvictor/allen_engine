// Adapted from https://github.com/jherico/Vulkan/blob/cpp/base/vks/allocation.hpp
#pragma once

#include "device.hpp"
#include <vulkan/vulkan.hpp>

namespace vkg
{

// A wrapper class for an allocation, either an Image or Buffer.  Not intended to be used used directly
// but only as a base class providing common functionality for the classes below.
//
// Provides easy to use mechanisms for mapping, unmapping and copying host data to the device memory
struct Allocation
{
    std::shared_ptr<core::Device> m_pDevice;
    vk::UniqueDeviceMemory m_memory;
    vk::DeviceSize m_size;

    void* m_mapped = nullptr;

    // TODO: Default to own size attribute
    template <typename T = void>
    inline T* Map(size_t offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE)
    {
        m_mapped = m_pDevice->logical->mapMemory(m_memory.get(), offset, size, vk::MemoryMapFlags());
        return (T*) m_mapped;
    }

    inline void Unmap()
    {
        m_pDevice->logical->unmapMemory(m_memory.get());
        m_mapped = nullptr;
    }

    // inline void copy(const void* data, size_t size, vk::DeviceSize offset = 0) const { memcpy(static_cast<uint8_t*>(m_mapped) + offset, data, size); }
    inline void Copy(const void* data, size_t size) const { memcpy(m_mapped, data, size); }

    // template <typename T>
    // inline void copy(const T& data, vk::DeviceSize offset = 0) const {
    //     copy(&data, sizeof(T), offset);
    // }

    // template <typename T>
    // inline void copy(const std::vector<T>& data, vk::DeviceSize offset = 0) const {
    //     copy(sizeof(T) * data.size(), data.data(), offset);
    // }

    /**
        * Flush a memory range of the buffer to make it visible to the device
        *
        * @note Only required for non-coherent memory
        *
        * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
        * @param offset (Optional) Byte offset from beginning
        *
        * @return VkResult of the flush call
        */
    void Flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
    {
        return m_pDevice->logical->flushMappedMemoryRanges(vk::MappedMemoryRange{m_memory.get(), offset, size});
    }

    /**
        * Invalidate a memory range of the buffer to make it visible to the host
        *
        * @note Only required for non-coherent memory
        *
        * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
        * @param offset (Optional) Byte offset from beginning
        *
        * @return VkResult of the invalidate call
        */
    void Invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
    {
        return m_pDevice->logical->invalidateMappedMemoryRanges(vk::MappedMemoryRange{m_memory.get(), offset, size});
    }

    virtual void Allocate(const vk::MemoryRequirements& memRequirements, const vk::MemoryPropertyFlags& memProperties)
    {
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = m_pDevice->findMemoryType(memRequirements.memoryTypeBits, memProperties),
        };

        m_memory = m_pDevice->logical->allocateMemoryUnique(allocInfo, nullptr);
    }
};
} // namespace vkg
