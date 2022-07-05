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

    AssetHandle<IAsset> m_pAsset;
    GUID m_requesterEntityID;
    AssetLoader* m_pLoader = nullptr;

    Type m_type = Type::Invalid;
};

class AssetManager
{
    friend class Engine;

  private:
    std::map<std::type_index, std::unique_ptr<AssetLoader>> m_loaders;
    // TODO: Replace with dedicated per-type cache class
    std::map<AssetGUID, AssetHandle<IAsset>> m_assetCache;

    std::recursive_mutex m_mutex;
    std::vector<AssetRequest> m_pendingRequests;
    std::vector<AssetRequest> m_activeRequests;

    TaskService* m_pTaskService = nullptr;
    TaskSet m_loadingTask;
    bool m_isLoadingTaskRunning = false;

    AssetHandle<IAsset> GetInternal(const std::type_index& typeIndex, const std::string& path)
    {
        auto it = m_assetCache.try_emplace(path);
        if (it.second)
        {
            // Resource has not been created yet
            auto& pLoader = m_loaders.at(typeIndex);
            it.first->second = pLoader->Create(path);

            // Create dependencies as well
            for (auto& dep : it.first->second->m_dependencies)
            {
                auto& pDepLoader = m_loaders.at(dep.type);
                pDepLoader->Create(dep.id);
            }
        }

        return it.first->second;
    }

    AssetRequest* FindActiveRequest(const AssetGUID id)
    {
        std::lock_guard lock(m_mutex);

        // TODO: Use InvalidIndex
        AssetRequest* pRequest = nullptr;
        for (auto idx = 0; idx < m_activeRequests.size() && pRequest == nullptr; ++idx)
        {
            if (m_activeRequests[idx].m_pAsset->GetID() == id)
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
            auto pActiveRequest = FindActiveRequest(pendingRequest.m_pAsset->GetID());
            if (pActiveRequest != nullptr)
            {
                // A request for this asset is already active
                if (pActiveRequest->m_type == pendingRequest.m_type)
                {
                }
                else
                {
                    // TODO: Mismatching requests types
                }
            }
            else
            {
                // Add the request
                m_activeRequests.push_back(std::move(pendingRequest));
            }
        }

        m_pendingRequests.clear();

        // Handle active requests
        if (!m_activeRequests.empty())
        {
            m_pTaskService->ScheduleTask(&m_loadingTask);
            m_isLoadingTaskRunning = true;
        }
    }

    void HandleActiveRequests(TaskSetPartition range, uint32_t threadnum)
    {
        int32_t requestCount = (int32_t) m_activeRequests.size() - 1;
        for (auto idx = requestCount; idx >= 0; idx--)
        {
            bool requestComplete = false;
            auto pRequest = &m_activeRequests[idx];
            if (pRequest->m_type == AssetRequest::Type::Load)
            {
                requestComplete = pRequest->m_pLoader->LoadAsset(pRequest->m_pAsset);

                if (requestComplete)
                {
                    // Remove completed requests
                    m_activeRequests.erase(m_activeRequests.begin() + idx);
                }
            }
            else
            {
                pRequest->m_pLoader->UnloadAsset(pRequest->m_pAsset);
                m_activeRequests.erase(m_activeRequests.begin() + idx);
            }
        }
    }

    void LoadInternal(const std::type_index& typeIndex, AssetHandle<IAsset> pAsset)
    {
        std::lock_guard lock(m_mutex);

        // TODO: Dependencies could be found during loading (in cases where they were saved in the asset)
        for (auto& dep : pAsset->m_dependencies)
        {
            // Also resolve missing dependencies
            auto pDependencyHandle = GetInternal(dep.type, dep.id);
            LoadInternal(dep.type, pDependencyHandle);
        }

        // TODO: Delegate find the loader to when we mark the requests as active (they can still be discarded in between)
        auto& pLoader = m_loaders.at(typeIndex);
        AssetRequest& request = m_pendingRequests.emplace_back();
        request.m_type = AssetRequest::Type::Load;
        request.m_pAsset = pAsset; // TODO: The manager class coud handle the load counts (pass IAsset*)
        request.m_pLoader = pLoader.get();
        // request.m_requesterEntityID // TODO
    }

    void UnloadInternal(std::type_index typeIndex, AssetHandle<IAsset> pAsset)
    {
        std::lock_guard lock(m_mutex);

        for (auto& dep : pAsset->m_dependencies)
        {
            auto pDependencyHandle = GetInternal(dep.type, dep.id);
            UnloadInternal(dep.type, pDependencyHandle);
        }

        auto& pLoader = m_loaders.at(typeIndex);
        AssetRequest& request = m_pendingRequests.emplace_back();
        request.m_type = AssetRequest::Type::Unload;
        request.m_pAsset = pAsset;
        request.m_pLoader = pLoader.get();
        // return pLoader->UnloadAsset(pAsset);
    }

    bool IsIdle() const
    {
        return m_pendingRequests.empty() && m_activeRequests.empty();
    }

  public:
    AssetManager(TaskService* pTaskService)
        : m_pTaskService(pTaskService),
          m_loadingTask(std::bind(&AssetManager::HandleActiveRequests, this, std::placeholders::_1, std::placeholders::_2)) {}

    ~AssetManager()
    {
        while (IsIdle())
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
    /// @todo Maybe forward arguments to the assets constructors ?
    template <AssetType T>
    AssetHandle<T> Get(std::string path)
    {
        return AssetHandle<T>(GetInternal(std::type_index(typeid(T)), path));
    }

    // TODO: We can get rid of the templatization since Asset know their types (in AssetID)
    template <AssetType T>
    void Load(AssetHandle<T> pAsset)
    {
        LoadInternal(std::type_index(typeid(T)), AssetHandle<IAsset>(pAsset));
    }

    template <AssetType T>
    void Unload(AssetHandle<T> pAsset)
    {
        UnloadInternal(std::type_index(typeid(T)), AssetHandle<IAsset>(pAsset));
    }
};
} // namespace aln