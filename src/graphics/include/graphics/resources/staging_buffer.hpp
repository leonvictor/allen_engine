#pragma once

#include "../command_buffer.hpp"
#include "buffer.hpp"
#include "image.hpp"

#include <common/containers/list.hpp>
#include <common/containers/vector.hpp>
#include <common/memory.hpp>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace aln
{

class RenderEngine;

/// @brief A GPU buffer accessible from the CPU used to stage memory during transfers. CPU -> Staging -> GPU
// This class allows client to reserve a portion of this buffer for their copying needs
// It is responsible for allocating memory ranges and "freeing" them when the transfers are complete
// We expect:
// * Max one transfer submit per frame, signaling the semaphore when it completes
// * First allocated, first freed
class StagingBuffer
{
  private:
    // Data
    vk::Device* m_pDevice;
    GPUBuffer m_buffer;

    // Allocator
    VmaVirtualBlock m_block;
    List<VmaVirtualAllocation> m_allocations;

    // Mapping
    std::byte* m_mapping;

    // Sync
    vk::Semaphore m_timelineSemaphore;
    uint64_t m_currentSemaphoreValue = 0;

  private:
    VkDeviceSize Allocate(size_t size)
    {
        VkDeviceSize offset;
        VmaVirtualAllocation alloc;
        VmaVirtualAllocationCreateInfo allocCreateInfo = {
            .size = size,
            .flags = VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT,
        };

        vmaVirtualAllocate(m_block, &allocCreateInfo, &alloc, &offset);

        if (offset == UINT64_MAX)
        {
            FreeCompleteTransfers();

            vmaVirtualAllocate(m_block, &allocCreateInfo, &alloc, &offset);
            assert(offset != UINT64_MAX); // Max capacity reached
        }

        auto pAllocSemaphoreValue = aln::New<uint64_t>(m_currentSemaphoreValue);
        vmaSetVirtualAllocationUserData(m_block, alloc, pAllocSemaphoreValue);

        m_allocations.push_back(alloc);

        return offset;
    }

    /// @brief Free all allocations whose transfer is complete
    void FreeCompleteTransfers()
    {
        assert(!m_allocations.empty());

        // Grab the most recent semaphore value (i.e. the index of the latest command submit)
        auto semaphoreValue = m_pDevice->getSemaphoreCounterValue(m_timelineSemaphore).value;

        VmaVirtualAllocationInfo allocInfo;
        auto pAlloc = m_allocations.begin();
        vmaGetVirtualAllocationInfo(m_block, *pAlloc, &allocInfo);
        uint64_t* pAllocSemaphoreValue = (uint64_t*) allocInfo.pUserData;

        while (pAlloc != m_allocations.end() && *pAllocSemaphoreValue < m_currentSemaphoreValue)
        {
            // Free alloc
            aln::Delete<uint64_t>(pAllocSemaphoreValue);
            vmaVirtualFree(m_block, *pAlloc);
            m_allocations.pop_front();

            // Go to the next one
            pAlloc = m_allocations.begin();
            
            // TODO: Will we test the empty list::end() ?
            assert(pAlloc != m_allocations.end());

            vmaGetVirtualAllocationInfo(m_block, *pAlloc, &allocInfo);
            pAllocSemaphoreValue = (uint64_t*) allocInfo.pUserData;
        }
    }

  public:
    void Initialize(RenderEngine* pRenderEngine, size_t size);

    void Shutdown()
    {
        m_pDevice->destroySemaphore(m_timelineSemaphore);

        for (auto& alloc : m_allocations)
        {
            VmaVirtualAllocationInfo allocInfo;
            vmaGetVirtualAllocationInfo(m_block, alloc, &allocInfo);

            aln::Delete<uint64_t>((uint64_t*&) allocInfo.pUserData);
            vmaVirtualFree(m_block, alloc);
        }
        m_allocations.clear();
        vmaDestroyVirtualBlock(m_block);

        m_mapping = nullptr;
        m_buffer.Shutdown();
    }

    void FrameUpdate()
    {
        m_currentSemaphoreValue++;
    }

    /// @brief Upload image data to a GPU image going through the staging buffer. Returns the semaphore that will be signaled when transfer is complete as well as the value to expect, in case user want to wait for it
    template <typename CommandBufferType, typename DataType>
    std::pair<const vk::Semaphore*, uint64_t> UploadImageToGPU(const Vector<DataType>& srcData, GPUImage& dstImage, CommandBufferSubmission<CommandBufferType>& cbSubmission)
    {
        auto dataSize = srcData.size() * sizeof(DataType);
        auto offset = Allocate(dataSize);

        // CPU -> Staging
        memcpy(m_mapping + offset, srcData.data(), dataSize);

        // Staging -> GPU
        vk::BufferImageCopy2 region = {
            .bufferOffset = offset,
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {
                .width = dstImage.GetWidth(),
                .height = dstImage.GetHeight(),
                .depth = 1,
            },
        };

        vk::CopyBufferToImageInfo2 copyInfo = {
            .srcBuffer = m_buffer.GetVkBuffer(),
            .dstImage = dstImage.GetVkImage(),
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &region,
        };

        auto cb = (vk::CommandBuffer) *cbSubmission.GetCommandBuffer();
        cb.copyBufferToImage2(copyInfo);

        const auto commandCompleteSemaphoreValue = m_currentSemaphoreValue + 1;
        cbSubmission.SignalSemaphore(&m_timelineSemaphore, commandCompleteSemaphoreValue);

        return std::make_pair(&m_timelineSemaphore, commandCompleteSemaphoreValue);
    }

    /// @brief Upload some data to a GPU buffer going through the staging buffer. Returns the semaphore that will be signaled when transfer is complete as well as the value to expect, in case user want to wait for it
    template <typename CommandBufferType, typename DataType>
    std::pair<const vk::Semaphore*, uint64_t> UploadBufferToGPU(const Vector<DataType>& srcData, GPUBuffer& dstBuffer, CommandBufferSubmission<CommandBufferType>& cbSubmission)
    {
        assert(!srcData.empty() && dstBuffer.GetVkBuffer());

        auto dataSize = srcData.size() * sizeof(DataType);
        auto offset = Allocate(dataSize);

        // CPU -> Staging
        memcpy(m_mapping + offset, srcData.data(), dataSize);

        // Staging -> GPU
        vk::BufferCopy copyRegion = {
            .srcOffset = offset,
            .dstOffset = 0,
            .size = dataSize,
        };

        auto cb = (vk::CommandBuffer) *cbSubmission.GetCommandBuffer();
        cb.copyBuffer(m_buffer.GetVkBuffer(), dstBuffer.GetVkBuffer(), copyRegion);

        const auto commandCompleteSemaphoreValue = m_currentSemaphoreValue + 1;
        cbSubmission.SignalSemaphore(&m_timelineSemaphore, commandCompleteSemaphoreValue);

        return std::make_pair(&m_timelineSemaphore, commandCompleteSemaphoreValue);
    }
};
} // namespace aln