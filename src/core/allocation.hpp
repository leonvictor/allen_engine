// Adapted from https://github.com/jherico/Vulkan/blob/cpp/base/vks/allocation.hpp
#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"

namespace core {

// A wrapper class for an allocation, either an Image or Buffer.  Not intended to be used used directly
// but only as a base class providing common functionality for the classes below.
//
// Provides easy to use mechanisms for mapping, unmapping and copying host data to the device memory
struct Allocation {
    std::shared_ptr<core::Device> device;
    vk::DeviceMemory memory;
    vk::DeviceSize size{ 0 };
    vk::DeviceSize alignment{ 0 };
    vk::DeviceSize allocSize{ 0 };
    void* mapped{ nullptr };

    template <typename T = void>
    inline T* map(size_t offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) {
        mapped = device->logicalDevice.get().mapMemory(memory, offset, size, vk::MemoryMapFlags());
        return (T*)mapped;
    }

    inline void unmap() {
        device->logicalDevice.get().unmapMemory(memory);
        mapped = nullptr;
    }

    // inline void copy(const void* data, size_t size, vk::DeviceSize offset = 0) const { memcpy(static_cast<uint8_t*>(mapped) + offset, data, size); }
    inline void copy(const void* data, size_t size) const { memcpy(mapped, data, size); }

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
    void flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
        return device->logicalDevice.get().flushMappedMemoryRanges(vk::MappedMemoryRange{ memory, offset, size });
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
    void invalidate(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) {
        return device->logicalDevice.get().invalidateMappedMemoryRanges(vk::MappedMemoryRange{ memory, offset, size });
    }

    virtual void allocate(const vk::MemoryRequirements& memRequirements, const vk::MemoryPropertyFlags& memProperties)
    {
        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, memProperties);

        memory = device->logicalDevice.get().allocateMemory(allocInfo, nullptr);
    }

    virtual void destroy() {
        if (nullptr != mapped) {
            unmap();
        }
        if (memory) {
            device->logicalDevice.get().freeMemory(memory);
            memory = vk::DeviceMemory();
        }
    }
};
}  // namespace vks
