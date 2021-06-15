// Adapted from https://github.com/jherico/Vulkan/blob/cpp/base/vks/allocation.hpp
#pragma once

#include "../device.hpp"
#include <vulkan/vulkan.hpp>

// TODO: Make a class w/ private fields
namespace vkg
{

/// @brief A wrapper class for an allocation, either an Image or Buffer.  Not intended to be used used directly
/// but only as a base class providing common functionality for the classes below.
///
/// Provides easy to use mechanisms for mapping, unmapping and copying host data to the device memory
class Allocation
{
  protected:
    std::shared_ptr<Device> m_pDevice;
    vk::UniqueDeviceMemory m_memory;
    vk::DeviceSize m_size;

    void* m_mapped = nullptr;

    virtual void Allocate(const vk::MemoryRequirements& memRequirements, const vk::MemoryPropertyFlags& memProperties)
    {
        vk::MemoryAllocateInfo allocInfo =
            {
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = m_pDevice->FindMemoryType(memRequirements.memoryTypeBits, memProperties),
            };

        m_memory = m_pDevice->GetVkDevice().allocateMemoryUnique(allocInfo, nullptr);
    }

  public:
    Allocation() = default;

    // No copy allowed
    Allocation(const Allocation&) = delete;
    Allocation& operator=(const Allocation&) = delete;

    // Move assignement
    Allocation& operator=(Allocation&& other)
    {
        if (this != &other)
        {
            m_pDevice = std::move(other.m_pDevice);
            m_memory = std::move(other.m_memory);

            m_size = other.m_size;
            m_mapped = other.m_mapped;
        }
        return *this;
    }

    // Move constructor
    Allocation(Allocation&& other)
    {
        m_pDevice = std::move(other.m_pDevice);
        m_memory = std::move(other.m_memory);

        m_size = other.m_size;
        m_mapped = other.m_mapped;
    }

    vk::DeviceSize GetSize() const { return m_size; }

    // TODO: Default to own size attribute
    template <typename T = void>
    inline T* Map(size_t offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE)
    {
        m_mapped = m_pDevice->GetVkDevice().mapMemory(m_memory.get(), offset, size, vk::MemoryMapFlags());
        return (T*) m_mapped;
    }

    inline void Unmap()
    {
        m_pDevice->GetVkDevice().unmapMemory(m_memory.get());
        m_mapped = nullptr;
    }

    inline void Copy(const void* data, size_t size, vk::DeviceSize offset = 0) const
    {
        memcpy(static_cast<uint8_t*>(m_mapped) + offset, data, size);
    }

    // template <typename T>
    // inline void Copy(const T& data, vk::DeviceSize offset = 0) const
    // {
    //     Copy(&data, sizeof(T), offset);
    // }

    template <typename T>
    inline void Copy(const std::vector<T>& data, vk::DeviceSize offset = 0) const
    {
        Copy(data.data(), sizeof(T) * data.size(), offset);
    }

    /// @brief Flush a memory range of the buffer to make it visible to the device
    ///
    /// @note Only required for non-coherent memory
    ///
    /// @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
    /// @param offset (Optional) Byte offset from beginning
    ///
    /// @return VkResult of the flush call
    ///
    void Flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
    {
        vk::MappedMemoryRange mappedRange;
        mappedRange.memory = m_memory.get();
        mappedRange.offset = offset;
        mappedRange.size = size;
        return m_pDevice->GetVkDevice().flushMappedMemoryRanges(mappedRange);
    }

    /// @brief Invalidate a memory range of the buffer to make it visible to the host
    ///
    /// @note Only required for non-coherent memory
    ///
    /// @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
    /// @param offset (Optional) Byte offset from beginning
    ///
    /// @return VkResult of the invalidate call
    ///
    void Invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
    {
        vk::MappedMemoryRange mappedRange;
        mappedRange.memory = m_memory.get();
        mappedRange.offset = offset;
        mappedRange.size = size;
        return m_pDevice->GetVkDevice().invalidateMappedMemoryRanges(mappedRange);
    }
};
} // namespace vkg
