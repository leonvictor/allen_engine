#pragma once

#include <algorithm>
#include <concepts>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <typeinfo>

#include <common/threading/task_service.hpp>

#include "asset.hpp"
#include "handle.hpp"
#include "loader.hpp"
#include "record.hpp"

namespace aln
{

/// @todo Handle asset lifetime. for now they're all "instant" (name subject to change)
enum class AssetLifetime
{
    Instant, // Unloaded as soon as it's not used anymore
    Level,   // Unloaded when a level ends
    Global,  // Unloaded on game exit
};

struct AssetRequest
{
    enum class Type : uint8_t
    {
        Load,
        Unload,
        Invalid,
    };

    GUID m_requesterEntityID;
    AssetRecord* m_pAssetRecord = nullptr;
    AssetLoader* m_pLoader = nullptr;

    Type m_type = Type::Invalid;

    bool IsValid() const { return m_type != Type::Invalid; }
    bool IsLoadingRequest() const { return m_type == Type::Load; }
    bool IsUnloadingRequest() const { return m_type == Type::Unload; }
};

class AssetManager
{
    friend class Engine;

  private:
    std::map<std::type_index, std::unique_ptr<AssetLoader>> m_loaders;
    std::map<AssetGUID, AssetRecord> m_assetCache;

    std::recursive_mutex m_mutex;
    std::vector<AssetRequest> m_pendingRequests;
    std::vector<AssetRequest> m_activeRequests;

    TaskService* m_pTaskService = nullptr;
    TaskSet m_loadingTask;
    bool m_isLoadingTaskRunning = false;

    /// @brief Find an existing record. The record must have already been created !
    /// @todo Use AssetID
    AssetRecord* FindRecord(const std::string& path)
    {
        auto it = m_assetCache.find(path);
        assert(it != m_assetCache.end());
        return &it->second;
    }

    AssetRecord* GetOrCreateRecord(const std::type_index& typeIndex, const std::string& path)
    {
        auto it = m_assetCache.try_emplace(path);
        if (it.second)
        {
            // Resource has not been created yet
            auto& pLoader = m_loaders.at(typeIndex);
            it.first->second.m_pAsset = pLoader->Create(path);
            // Create dependencies as well
            for (auto& dep : it.first->second.m_pAsset->m_dependencies)
            {
                auto& pDepLoader = m_loaders.at(dep.type);
                // TODO:
                pDepLoader->Create(dep.id);
            }
        }

        return &it.first->second;
    }

    AssetRequest* FindActiveRequest(const AssetGUID id)
    {
        std::lock_guard lock(m_mutex);

        // TODO: Use InvalidIndex
        AssetRequest* pRequest = nullptr;
        for (auto idx = 0; idx < m_activeRequests.size() && pRequest == nullptr; ++idx)
        {
            if (m_activeRequests[idx].m_pAssetRecord->GetID() == id)
            {
                pRequest = &m_activeRequests[idx];
            }
        }
        return pRequest;
    }

    /// @brief Handle pending requests
    void Update()
    {
        if (m_isLoadingTaskRunning && !m_loadingTask.GetIsComplete())
        {
            return;
        }

        m_isLoadingTaskRunning = false;

        std::lock_guard lock(m_mutex);

        for (auto& pendingRequest : m_pendingRequests)
        {
            assert(pendingRequest.IsValid());

            auto pActiveRequest = FindActiveRequest(pendingRequest.m_pAssetRecord->GetID());

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
                    /// @todo move status to record
                    if (pendingRequest.m_pAssetRecord->m_pAsset->IsLoaded())
                    {
                        // TODO: Asset already loaded
                    }
                    else
                    {
                        // Add the request
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
                    if (pendingRequest.m_pAssetRecord->m_pAsset->IsUnloaded())
                    {
                        // TODO: Asset already unloaded
                    }
                    else
                    {
                        // Add the request
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

    void HandleActiveRequests()
    {
        int32_t requestCount = (int32_t) m_activeRequests.size() - 1;
        for (auto idx = requestCount; idx >= 0; idx--)
        {
            bool requestComplete = false;
            auto pRequest = &m_activeRequests[idx];
            if (pRequest->m_type == AssetRequest::Type::Load)
            {
                requestComplete = pRequest->m_pLoader->Load(pRequest->m_pAssetRecord);

                if (requestComplete)
                {
                    pRequest->m_pAssetRecord->m_pAsset->m_status = IAsset::Status::Loaded;
                    // Remove completed requests
                    m_activeRequests.erase(m_activeRequests.begin() + idx);
                }
            }
            else
            {
                pRequest->m_pLoader->Unload(pRequest->m_pAssetRecord);
                pRequest->m_pAssetRecord->m_pAsset->m_status = IAsset::Status::Unloaded;
                m_activeRequests.erase(m_activeRequests.begin() + idx);
            }
        }
    }

    void LoadInternal(const std::type_index& typeIndex, AssetRecord* pRecord)
    {
        std::lock_guard lock(m_mutex);

        auto pAsset = pRecord->GetAsset();
        // TODO: Dependencies could be found during loading (in cases where they were saved in the asset)
        for (auto& dep : pAsset->m_dependencies)
        {
            // Also resolve missing dependencies
            auto pDependencyRecord = FindRecord(dep.id);
            LoadInternal(dep.type, pDependencyRecord);
        }

        // TODO: Delegate finding the loader to when we mark the requests as active (they can still be discarded in between)
        auto& pLoader = m_loaders.at(typeIndex);
        AssetRequest& request = m_pendingRequests.emplace_back();
        request.m_type = AssetRequest::Type::Load;
        request.m_pAssetRecord = pRecord;
        request.m_pLoader = pLoader.get();
        // request.m_requesterEntityID // TODO
    }

    void UnloadInternal(std::type_index typeIndex, AssetRecord* pRecord)
    {
        std::lock_guard lock(m_mutex);

        auto pAsset = pRecord->GetAsset();
        for (auto& dep : pAsset->m_dependencies)
        {
            auto pDependencyRecord = FindRecord(dep.id);
            UnloadInternal(dep.type, pDependencyRecord);
        }

        auto& pLoader = m_loaders.at(typeIndex);
        AssetRequest& request = m_pendingRequests.emplace_back();
        request.m_type = AssetRequest::Type::Unload;
        request.m_pAssetRecord = pRecord;
        request.m_pLoader = pLoader.get();
    }

    bool IsIdle() const { return m_pendingRequests.empty() && m_activeRequests.empty(); }
    bool IsBusy() const { return !IsIdle(); }

  public:
    AssetManager(TaskService* pTaskService)
        : m_pTaskService(pTaskService),
          m_loadingTask([this](TaskSetPartition, uint32_t)
              { HandleActiveRequests(); }) {}

    ~AssetManager()
    {
        while (IsBusy())
        {
            Update();
        }
    }

    /// @brief Register a new loader
    /// @tparam T: Asset type
    /// @tparam TLoader: Loader type
    template <AssetType T, typename TLoader, class... Args>
    void RegisterAssetLoader(Args... args)
    {
        static_assert(std::is_base_of_v<AssetLoader, TLoader>);

        auto it = m_loaders.try_emplace(std::type_index(typeid(T)), nullptr);
        if (it.second)
        {
            it.first->second = std::make_unique<TLoader>(args...);
        }
    }

    /// @brief Return a pointer to the asset corresponding to the given path (todo: key).
    /// If the asset was previously requested, a pointer to the existing instance will be returned. Otherwise the asset will be created and cached.
    /// @todo Probably abstract away the path so that the manager chooses the loading mechanism (and not game code)
    /// @todo Maybe forward arguments to the assets constructors ? -> No: Asset are pre-baked so we don't use parameters or constructors.
    template <AssetType T>
    AssetHandle<T> Get(std::string path)
    {
        auto pRecord = GetOrCreateRecord(std::type_index(typeid(T)), path);
        return AssetHandle<T>(pRecord->GetAsset<T>());
    }

    // TODO: We can get rid of the templatization since Asset know their types (in AssetID)
    template <AssetType T>
    void Load(AssetHandle<T> pAsset)
    {
        auto pRecord = FindRecord(pAsset->GetID());
        pRecord->AddReference();
        // TODO: ?
        if (pRecord->GetReferenceCount() == 1)
        {
            LoadInternal(std::type_index(typeid(T)), pRecord);
        }
    }

    template <AssetType T>
    void Unload(AssetHandle<T> pAsset)
    {
        auto pRecord = FindRecord(pAsset->GetID());
        if (pRecord->GetReferenceCount() == 1)
        {
            UnloadInternal(std::type_index(typeid(T)), pRecord);
        }
        pRecord->RemoveReference();
    }
};
} // namespace aln