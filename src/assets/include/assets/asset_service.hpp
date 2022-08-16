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
#include "request.hpp"

namespace aln
{

/// @todo Handle asset lifetime. for now they're all "instant" (name subject to change)
enum class AssetLifetime
{
    Instant, // Unloaded as soon as it's not used anymore
    Level,   // Unloaded when a level ends
    Global,  // Unloaded on game exit
};

/// @todo : Rename to service
class AssetService
{
    friend class Engine;
    friend struct AssetRequest;

  private:
    std::map<AssetTypeID, std::unique_ptr<AssetLoader>> m_loaders;
    std::map<AssetID, AssetRecord> m_assetCache;

    std::recursive_mutex m_mutex;
    std::vector<AssetRequest> m_pendingRequests;
    std::vector<AssetRequest> m_activeRequests;

    TaskService* m_pTaskService = nullptr;
    TaskSet m_loadingTask;
    bool m_isLoadingTaskRunning = false;

    /// @brief Find an existing record. The record must have already been created !
    AssetRecord* FindRecord(const AssetID& assetID);
    AssetRecord* GetOrCreateRecord(const AssetID& assetID);
    AssetRequest* FindActiveRequest(const AssetID id);

    /// @brief Handle pending requests
    void Update();
    void HandleActiveRequests();

    inline bool IsIdle() const { return m_pendingRequests.empty() && m_activeRequests.empty(); }
    inline bool IsBusy() const { return !IsIdle(); }

  public:
    AssetService(TaskService* pTaskService)
        : m_pTaskService(pTaskService),
          m_loadingTask([this](TaskSetPartition, uint32_t)
              { HandleActiveRequests(); }) {}

    ~AssetService()
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

        auto it = m_loaders.try_emplace(T::GetStaticAssetTypeID(), nullptr); // todo: remove nullptr ?
        if (it.second)
        {
            it.first->second = std::make_unique<TLoader>(args...);
        }
    }

    template <AssetType T>
    const IAssetLoader<T>* GetLoader()
    {
        return static_pointer_cast<AssetLoader<T>>(&m_loaders[T::GetStaticAssetTypeID()]);
    }

    void Load(IAssetHandle& assetHandle);
    void Unload(IAssetHandle& assetHandle);
};
} // namespace aln