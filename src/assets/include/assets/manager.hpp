#pragma once

#include <map>
#include <memory>
#include <typeindex>
#include <typeinfo>

#include "asset.hpp"
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
    std::map<std::type_index, std::unique_ptr<IAssetLoader>> m_loaders;
    // TODO: Replace with dedicated per-type cache class
    std::map<AssetGUID, std::shared_ptr<IAsset>> m_assetCache;

  public:
    /// @brief Register a new loader
    /// @tparam T: Asset type
    /// @tparam TLoader: Loader type
    template <typename T, typename TLoader, class... Args>
    void RegisterAssetLoader(Args... args)
    {
        static_assert(std::is_base_of_v<IAsset, T>);
        static_assert(std::is_base_of_v<IAssetLoader, TLoader>);

        auto it = m_loaders.try_emplace(std::type_index(typeid(T)), nullptr);

        if (it.second)
        {
            auto pLoader = std::make_unique<TLoader>(args...);
            it.first->second = std::move(pLoader);
        }
    }

    /// @brief Return a pointer to the asset corresponding to the given path (todo: key).
    /// If the asset was previously requested, an handle to the same asset will be returned. Otherwise the asset will be cached.
    /// @todo Probably abstract away the path so that the manager chooses the loading mechanism (and not game code)
    /// @todo Maybe forward arguments to the assets constructors ?
    template <typename T>
    std::shared_ptr<T> Get(std::string path)
    {
        static_assert(std::is_base_of_v<IAsset, T>);

        auto it = m_assetCache.try_emplace(path, nullptr);
        if (it.second)
        {
            // Resource has not been created yet
            auto loaderIt = m_loaders.find(std::type_index(typeid(T)));
            assert(loaderIt != m_loaders.end());

            it.first->second = loaderIt->second->Create(path);
        }
        return std::static_pointer_cast<T>(it.first->second);
    }
};
} // namespace aln