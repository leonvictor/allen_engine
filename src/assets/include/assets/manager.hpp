#pragma once

#include <concepts>
#include <map>
#include <memory>
#include <typeindex>
#include <typeinfo>

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

class AssetManager
{
  private:
    std::map<std::type_index, std::unique_ptr<AssetLoader>> m_loaders;
    // TODO: Replace with dedicated per-type cache class
    std::map<AssetID, AssetHandle<IAsset>> m_assetCache;

    AssetHandle<IAsset> GetInternal(const std::type_index& typeIndex, const AssetID& path)
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

    bool LoadInternal(const std::type_index& typeIndex, AssetHandle<IAsset> pAsset)
    {
        for (auto& dep : pAsset->m_dependencies)
        {
            // Also resolve missing dependencies
            auto pDependencyHandle = GetInternal(dep.type, dep.id);
            LoadInternal(dep.type, pDependencyHandle);
        }

        auto& pLoader = m_loaders.at(typeIndex);
        return pLoader->LoadAsset(pAsset);
    }

    void UnloadInternal(std::type_index typeIndex, AssetHandle<IAsset> pAsset)
    {
        for (auto& dep : pAsset->m_dependencies)
        {
            auto& pDependencyHandle = m_assetCache.at(dep.id);
            UnloadInternal(dep.type, pDependencyHandle);
        }

        auto& pLoader = m_loaders.at(typeIndex);
        return pLoader->UnloadAsset(pAsset);
    }

    void InitializeInternal(std::type_index typeIndex, AssetHandle<IAsset> pAsset)
    {
        for (auto& dep : pAsset->m_dependencies)
        {
            auto& pDependencyHandle = m_assetCache.at(dep.id);
            InitializeInternal(dep.type, pDependencyHandle);
        }

        auto& pLoader = m_loaders.at(typeIndex);
        pLoader->InitializeAsset(pAsset);
    }

    void ShutdownInternal(std::type_index typeIndex, AssetHandle<IAsset> pAsset)
    {
        for (auto& dep : pAsset->m_dependencies)
        {
            auto& pDependencyHandle = m_assetCache.at(dep.id);
            ShutdownInternal(dep.type, pDependencyHandle);
        }

        auto& pLoader = m_loaders.at(typeIndex);
        pLoader->ShutdownAsset(pAsset);
    }

  public:
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

    template <AssetType T>
    const IAssetLoader<T>* GetLoader()
    {
        return static_pointer_cast<AssetLoader<T>>(&m_loaders[std::type_index(typeid(T))]);
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

    template <AssetType T>
    bool Load(AssetHandle<T> pAsset)
    {
        return LoadInternal(std::type_index(typeid(T)), AssetHandle<IAsset>(pAsset));
    }

    template <AssetType T>
    void Unload(AssetHandle<T> pAsset)
    {
        UnloadInternal(std::type_index(typeid(T)), AssetHandle<IAsset>(pAsset));
    }

    template <AssetType T>
    void Initialize(AssetHandle<T> pAsset)
    {
        InitializeInternal(std::type_index(typeid(T)), AssetHandle<IAsset>(pAsset));
    }

    template <AssetType T>
    void Shutdown(AssetHandle<T> pAsset)
    {
        ShutdownInternal(std::type_index(typeid(T)), AssetHandle<IAsset>(pAsset));
    }
};
} // namespace aln