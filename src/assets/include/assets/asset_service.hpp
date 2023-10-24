#pragma once

#include "asset.hpp"
#include "handle.hpp"
#include "loader.hpp"
#include "record.hpp"
#include "request.hpp"

#include <common/containers/array.hpp>
#include <common/containers/hash_map.hpp>
#include <common/threading/task_service.hpp>
#include <graphics/render_engine.hpp>

#include <mutex>

#include <vulkan/vulkan.hpp>

namespace aln
{

/// @todo Handle asset lifetime. for now they're all "instant" (name subject to change)
enum class AssetLifetime
{
    Instant, // Unloaded as soon as it's not used anymore
    Level,   // Unloaded when a level ends
    Global,  // Unloaded on game exit
};

class AssetService : public IService
{
    friend class Engine;
    friend struct AssetRequest;

  private:
    HashMap<AssetTypeID, std::unique_ptr<IAssetLoader>> m_loaders;
    HashMap<AssetID, AssetRecord> m_assetCache;

    Vector<AssetRequest> m_pendingRequests;
    Vector<AssetRequest> m_activeRequests;

    RenderEngine* m_pRenderDevice = nullptr;

    std::recursive_mutex m_mutex;
    TaskService* m_pTaskService = nullptr;
    TaskSet m_loadingTask;
    bool m_isLoadingTaskRunning = false;

    TransferQueuePersistentCommandBuffer m_transferCommandBuffer;
    GraphicsQueuePersistentCommandBuffer m_graphicsCommandBuffer;

    /// @brief Find an existing record. The record must have already been created !
    AssetRecord* FindRecord(const AssetID& assetID);
    AssetRecord* GetOrCreateRecord(const AssetID& assetID);
    AssetRequest* FindActiveRequest(const AssetID id);

    /// @brief Handle pending requests
    void Update();
    void HandleActiveRequests(uint32_t threadIdx);

    inline bool IsIdle() const { return m_pendingRequests.empty() && m_activeRequests.empty() && !m_isLoadingTaskRunning; }
    inline bool IsBusy() const { return !IsIdle(); }

  public:
    AssetService() : m_loadingTask([this](TaskSetPartition range, uint32_t threadIdx)
                         { HandleActiveRequests(threadIdx); }) {}

    void Initialize(TaskService& taskService, RenderEngine& pRenderEngine)
    {
        m_pTaskService = &taskService;
        m_pRenderDevice = &pRenderEngine;
    }

    void Shutdown()
    {
        while (IsBusy())
        {
            Update();
        }

        // TODO: Properly remove cache entries when the last reference is unloaded
        // assert(m_assetCache.empty());
        m_loaders.clear();
    }

    /// @brief Register a new loader
    /// @tparam T: Asset type
    /// @tparam TLoader: Loader type
    template <AssetType T, typename TLoader, class... Args>
    void RegisterAssetLoader(Args... args)
    {
        static_assert(std::is_base_of_v<IAssetLoader, TLoader>);

        auto it = m_loaders.try_emplace(T::GetStaticAssetTypeID(), nullptr); // todo: remove nullptr ?
        if (it.second)
        {
            it.first->second = std::make_unique<TLoader>(args...);
        }
    }

    template <AssetType T>
    const IAssetLoader* GetLoader()
    {
        return &m_loaders[T::GetStaticAssetTypeID()];
    }

    void Load(IAssetHandle& assetHandle);
    void Unload(IAssetHandle& assetHandle);
};
} // namespace aln