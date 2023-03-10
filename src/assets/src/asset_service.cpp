#pragma once

#include "asset_service.hpp"

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
    if (m_isLoadingTaskRunning && !m_loadingTask.GetIsComplete())
    {
        return;
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
        // TODO: Enable multi-threading when the rendering engine is ready for it
        // m_pTaskService->ScheduleTask(&m_loadingTask);
        HandleActiveRequests();
        m_isLoadingTaskRunning = true;
    }
}

void AssetService::HandleActiveRequests()
{
    int32_t requestCount = (int32_t) m_activeRequests.size() - 1;
    for (auto idx = requestCount; idx >= 0; idx--)
    {
        auto pRequest = &m_activeRequests[idx];
        pRequest->m_pLoader = m_loaders.at(pRequest->m_pAssetRecord->GetAssetTypeID()).get();
        pRequest->m_requestAssetLoad = std::bind(&AssetService::Load, this, std::placeholders::_1);
        pRequest->m_requestAssetUnload = std::bind(&AssetService::Unload, this, std::placeholders::_1);

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