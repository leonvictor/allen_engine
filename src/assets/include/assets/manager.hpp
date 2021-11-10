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
    std::map<AssetGUID, AssetHandle<IAsset>> m_assetCache;

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

    /// @brief Return a pointer to the asset corresponding to the given path (todo: key).
    /// If the asset was previously requested, a pointer to the existing instance will be returned. Otherwise the asset will be created and cached.
    /// @todo Probably abstract away the path so that the manager chooses the loading mechanism (and not game code)
    /// @todo Maybe forward arguments to the assets constructors ?
    template <AssetType T>
    AssetHandle<T> Get(std::string path)
    {
        auto it = m_assetCache.try_emplace(path, nullptr);
        if (it.second)
        {
            // Resource has not been created yet
            auto loaderIt = m_loaders.find(std::type_index(typeid(T)));
            assert(loaderIt != m_loaders.end());

            it.first->second = loaderIt->second->Create(path);
        }

        return AssetHandle<T>(it.first->second);
    }

    template <AssetType T>
    bool Load(AssetHandle<T> pAsset)
    {
        auto& pLoader = m_loaders.at(std::type_index(typeid(T)));
        return pLoader->LoadAsset(AssetHandle<IAsset>(pAsset));
    }

    template <AssetType T>
    void Unload(AssetHandle<T> pAsset)
    {
        auto& pLoader = m_loaders.at(std::type_index(typeid(T)));
        pLoader->UnloadAsset(AssetHandle<IAsset>(pAsset));
    }

    template <AssetType T>
    void Initialize(AssetHandle<T> pAsset)
    {
        auto& pLoader = m_loaders.at(std::type_index(typeid(T)));
        pLoader->InitializeAsset(AssetHandle<IAsset>(pAsset));
    }

    template <AssetType T>
    void Shutdown(AssetHandle<T> pAsset)
    {
        auto& pLoader = m_loaders.at(std::type_index(typeid(T)));
        pLoader->ShutdownAsset(AssetHandle<IAsset>(pAsset));
    }
};
} // namespace aln