#pragma once

#include "asset_service.hpp"

#include <vulkan/vulkan.hpp>

namespace aln
{

AssetRecord* AssetService::FindRecord(const AssetID& assetID)
{
    auto it = m_assetCache.find(assetID);
    assert(it != m_assetCache.end());
    return &it->second;
}

AssetRecord* AssetService::GetOrCreateRecord(const AssetID& assetID)
{
    auto it = m_assetCache.try_emplace(assetID, assetID);
    // TODO: This condition might not be necessary ?
    if (it.second)
    {
        it.first->second = AssetRecord(assetID);
    }

    return &it.first->second;
}

AssetRequest* AssetService::FindActiveRequest(const AssetID id)
{
    std::lock_guard lock(m_mutex);

    AssetRequest* pRequest = nullptr;
    for (auto idx = 0; idx < m_activeRequests.size() && pRequest == nullptr; ++idx)
    {
        if (m_activeRequests[idx].m_pAssetRecord->GetAssetID() == id)
        {
            pRequest = &m_activeRequests[idx];
        }
    }
    return pRequest;
}

/// @brief Handle pending requests
void AssetService::Update()
{
    ZoneScoped;

    if (m_isLoadingTaskRunning)
    {
        if (m_loadingTask.GetIsComplete())
        {
            // Submit all the recorded command buffers (mostly cpu->gpu transfers tasks)
            // Grab all semaphore that need notifying
            Vector<vk::Semaphore> transferQueueSemaphores;
            Vector<vk::Semaphore> graphicsQueueSemaphores;

            for (auto& request : m_activeRequests)
            {
                if (request.m_pTransferQueueCommandsSemaphore && !request.m_commandBuffersSubmitted)
                {
                    transferQueueSemaphores.push_back(*request.m_pTransferQueueCommandsSemaphore);
                    request.m_commandBuffersSubmitted = true;
                }

                if (request.m_pGraphicsQueueCommandsSemaphore && !request.m_commandBuffersSubmitted)
                {
                    graphicsQueueSemaphores.push_back(*request.m_pGraphicsQueueCommandsSemaphore);
                    request.m_commandBuffersSubmitted = true;
                }
            }

            if (!transferQueueSemaphores.empty())
            {
                Vector<uint64_t> values(transferQueueSemaphores.size(), 1);

                Queue::SubmissionRequest request;
                request.ExecuteCommandBuffer(m_transferCommandBuffer);
                request.SignalSemaphores(transferQueueSemaphores, 1);
                
                m_pRenderDevice->GetTransferQueue().Submit(request, vk::Fence{});
            }
            else
            {
                m_transferCommandBuffer.Release();
            }

            if (!graphicsQueueSemaphores.empty())
            {
                Vector<uint64_t> values(graphicsQueueSemaphores.size(), 1);

                Queue::SubmissionRequest request;
                request.ExecuteCommandBuffer(m_graphicsCommandBuffer);
                request.SignalSemaphores(graphicsQueueSemaphores, 1);

                m_pRenderDevice->GetGraphicsQueue().Submit(request, vk::Fence{});
            }
            else
            {
                m_graphicsCommandBuffer.Release();
            }
        }
        else
        {
            return;
        }
    }

    m_isLoadingTaskRunning = false;

    std::lock_guard lock(m_mutex);

    // Filter pending requests and move them to active according to their status
    for (auto& pendingRequest : m_pendingRequests)
    {
        assert(pendingRequest.IsValid());

        if (pendingRequest.m_pAssetRecord->GetAssetPath().empty())
        {
            pendingRequest.m_status = AssetRequest::State::Complete;
            pendingRequest.m_pAssetRecord->m_status = AssetStatus::LoadingFailed;
            continue;
        }

        auto pActiveRequest = FindActiveRequest(pendingRequest.m_pAssetRecord->GetAssetID());
        if (pendingRequest.IsLoadingRequest())
        {
            if (pActiveRequest != nullptr)
            {
                // A request for this asset is already active
                if (pActiveRequest->IsUnloadingRequest())
                {
                    // TODO: Handle mismatching requests types
                }
            }
            else
            {
                if (pendingRequest.m_pAssetRecord->IsLoaded())
                {
                    // TODO: Asset already loaded
                }
                else
                {
                    // Add the request
                    pendingRequest.m_status = AssetRequest::State::Loading;
                    m_activeRequests.push_back(std::move(pendingRequest));
                }
            }
        }
        else // Unloading request
        {
            if (pActiveRequest != nullptr)
            {
                // A request for this asset is already active
                if (pActiveRequest->IsLoadingRequest())
                {
                    // TODO: Handle mismatching requests types
                }
            }
            else
            {
                if (pendingRequest.m_pAssetRecord->IsUnloaded())
                {
                    // TODO: Asset already unloaded
                }
                else
                {
                    // Add the request
                    pendingRequest.m_status = AssetRequest::State::Unloading;
                    m_activeRequests.push_back(std::move(pendingRequest));
                }
            }
        }
    }

    m_pendingRequests.clear();

    // Handle active requests
    if (!m_activeRequests.empty())
    {
        m_isLoadingTaskRunning = true;
        m_pTaskService->ScheduleTask(&m_loadingTask);
    }
}

void AssetService::HandleActiveRequests(uint32_t threadIdx = 0)
{
    ZoneScoped;

    // Start command buffers that loaders will record to
    m_transferCommandBuffer = m_pRenderDevice->GetTransferPersistentCommandPool(threadIdx).GetCommandBuffer();
    m_graphicsCommandBuffer = m_pRenderDevice->GetGraphicsPersistentCommandPool(threadIdx).GetCommandBuffer();

    int32_t requestCount = (int32_t) m_activeRequests.size() - 1;
    for (auto idx = requestCount; idx >= 0; idx--)
    {
        auto pRequest = &m_activeRequests[idx];
        pRequest->m_pLoader = m_loaders.at(pRequest->m_pAssetRecord->GetAssetTypeID()).get();
        pRequest->m_requestAssetLoad = std::bind(&AssetService::Load, this, std::placeholders::_1);
        pRequest->m_requestAssetUnload = std::bind(&AssetService::Unload, this, std::placeholders::_1);
        pRequest->m_pRenderDevice = m_pRenderDevice;
        pRequest->m_threadIdx = threadIdx;
        pRequest->m_transferCommandBuffer = m_transferCommandBuffer;
        pRequest->m_graphicsCommandBuffer = m_graphicsCommandBuffer;

        if (pRequest->IsLoadingRequest())
        {
            switch (pRequest->m_status)
            {
            case AssetRequest::State::Loading:
            {
                pRequest->Load();
            }
            break;
            case AssetRequest::State::WaitingForDependencies:
            {
                pRequest->WaitForDependencies();
            }
            break;
            case AssetRequest::State::Installing:
            {
                pRequest->Install();
            }
            break;
            }
        }
        else
        {
            pRequest->Unload();
        }

        if (pRequest->IsComplete())
        {
            // Remove completed request
            m_activeRequests.erase(m_activeRequests.begin() + idx);
        }
    }
}

void AssetService::Load(IAssetHandle& assetHandle)
{
    if (!assetHandle.GetAssetID().IsValid())
    {
        return;
    }

    std::lock_guard lock(m_mutex);

    auto pRecord = GetOrCreateRecord(assetHandle.GetAssetID());
    // Update the handle
    assetHandle.m_pAssetRecord = pRecord;

    pRecord->AddReference();
    if (pRecord->GetReferenceCount() == 1)
    {
        AssetRequest& request = m_pendingRequests.emplace_back();
        request.m_type = AssetRequest::Type::Load;
        request.m_status = AssetRequest::State::Pending;
        request.m_pAssetRecord = pRecord;
        // request.m_requesterEntityID // TODO
    }
}

void AssetService::Unload(IAssetHandle& assetHandle)
{
    std::lock_guard lock(m_mutex);

    if (!assetHandle.GetAssetID().IsValid())
    {
        return;
    }

    assetHandle.m_pAssetRecord = nullptr;

    auto pRecord = FindRecord(assetHandle.GetAssetID());
    if (pRecord->GetReferenceCount() == 1)
    {
        AssetRequest& request = m_pendingRequests.emplace_back();
        request.m_type = AssetRequest::Type::Unload;
        request.m_status = AssetRequest::State::Pending;
        request.m_pAssetRecord = pRecord;
    }
    pRecord->RemoveReference();
}
} // namespace aln