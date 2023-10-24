#pragma once

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>

// TODO: Make a class w/ private fields
/// @note Adapted from https://github.com/jherico/Vulkan/blob/cpp/base/vks/allocation.hpp
namespace aln
{

class RenderEngine;

namespace resources
{

/// @brief A wrapper class for an allocation, either an Image or Buffer.  Not intended to be used used directly
/// but only as a base class providing common functionality for the classes below.
///
/// Provides easy to use mechanisms for mapping, unmapping and copying host data to the pRenderEngine memory
class Allocation
{
  protected:
    RenderEngine* m_pRenderEngine;
    vk::DeviceMemory m_memory;
    vk::DeviceSize m_size;

    void* m_mapped = nullptr;

    virtual void Allocate(const vk::MemoryRequirements& memRequirements, const vk::MemoryPropertyFlags& memProperties);

  public:
    Allocation() = default;

    // No copy allowed
    Allocation(const Allocation&) = delete;
    Allocation& operator=(const Allocation&) = delete;

    // Move assignement
    Allocation& operator=(Allocation&& other);

    // Move constructor
    Allocation(Allocation&& other);

    virtual void Shutdown();

    inline vk::DeviceSize GetSize() const { return m_size; }

    void Map(size_t offset = 0, vk::DeviceSize = vk::WholeSize);

    template <typename T = void>
    inline T* Map(size_t offset = 0, vk::DeviceSize size = vk::WholeSize)
    {
        Map(offset, size);
        return (T*) m_mapped;
    }

    void Unmap();

    inline void Copy(const void* data, size_t size, vk::DeviceSize offset = 0) const
    {
        assert(m_mapped != nullptr);
        memcpy(static_cast<uint8_t*>(m_mapped) + offset, data, size);
    }

    template <typename T>
    inline void Copy(const T& data, vk::DeviceSize offset = 0) const
    {
        Copy(&data, sizeof(T), offset);
    }

    template <typename T>
    inline void Copy(const Vector<T>& data, vk::DeviceSize offset = 0) const
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
    void Flush(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);

    /// @brief Invalidate a memory range of the buffer to make it visible to the host
    ///
    /// @note Only required for non-coherent memory
    ///
    /// @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
    /// @param offset (Optional) Byte offset from beginning
    ///
    /// @return VkResult of the invalidate call
    void Invalidate(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);
};
} // namespace resources
} // namespace aln
