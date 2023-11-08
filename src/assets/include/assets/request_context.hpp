#pragma once

#include <graphics/resources/staging_buffer.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{
class AssetRequestContext
{
    friend class AssetService;

  private:
    StagingBuffer* m_pStagingBuffer = nullptr;
    CommandBufferSubmission<TransferQueuePersistentCommandBuffer>* m_pTransferQueueSubmission = nullptr;
    CommandBufferSubmission<GraphicsQueuePersistentCommandBuffer>* m_pGraphicsQueueSubmission = nullptr;
    bool m_transferSubmissionAccessed = false;
    bool m_graphicsSubmissionAccessed = false;

  public:
    template<typename T>
    std::pair<const vk::Semaphore*, uint64_t> UploadBufferThroughStaging(const Vector<T>& data, GPUBuffer& dstBuffer)
    {
        assert(m_pStagingBuffer != nullptr && m_pTransferQueueSubmission != nullptr);
        
        m_transferSubmissionAccessed = true;
        return m_pStagingBuffer->UploadBufferToGPU<TransferQueuePersistentCommandBuffer, T>(data, dstBuffer, *m_pTransferQueueSubmission);
    }

    template<typename T>
    std::pair<const vk::Semaphore*, uint64_t> UploadImageThroughStaging(const Vector<T>& data, GPUImage& dstImage)
    {
        assert(m_pStagingBuffer != nullptr && m_pTransferQueueSubmission != nullptr);
        
        m_transferSubmissionAccessed = true;
        return m_pStagingBuffer->UploadImageToGPU(data, dstImage, *m_pTransferQueueSubmission);
    }

    CommandBufferSubmission<GraphicsQueuePersistentCommandBuffer>* GetGraphicsQueueSubmission() { 
        m_graphicsSubmissionAccessed = true;
        return m_pGraphicsQueueSubmission; 
    }

    CommandBufferSubmission<TransferQueuePersistentCommandBuffer>* GetTransferQueueSubmission()
    { 
        m_transferSubmissionAccessed = true;
        return m_pTransferQueueSubmission; 
    }

    bool WasTransferSubmissionAccessed() const { return m_transferSubmissionAccessed; }
    bool WasGraphicsSubmissionAccessed() const { return m_graphicsSubmissionAccessed; }
};
} // namespace aln