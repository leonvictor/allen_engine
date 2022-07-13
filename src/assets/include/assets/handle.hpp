#pragma once

#include "asset.hpp"

#include <common/memory.hpp>
#include <memory>

namespace aln
{
template <AssetType T>
class AssetHandle
{
    template <AssetType Other>
    friend class AssetHandle;

    friend class AssetLoader;
    friend class AssetManager;

  private:
    const T* m_pAsset = nullptr;

  public:
    AssetHandle() = default;
    AssetHandle(T* pAsset) : m_pAsset(pAsset) {}

    /// @brief Copy constructs a handled asset, sharing ownership
    /// @todo Only enable for this type, a derived one, or ??? the base IAsset class ???
    template <AssetType Other>
    AssetHandle(const AssetHandle<Other>& handle) : m_pAsset(std::static_pointer_cast<T>(handle.m_pAsset)) {}

    /// @brief Move constructor
    template <AssetType Other>
    AssetHandle(AssetHandle<Other>&& handle) : m_pAsset(std::move(std::static_pointer_cast<T>(handle.m_pAsset))) {}

    const T* get() const { return m_pAsset; }
    const T* operator->() const { return m_pAsset; }

    operator bool() const { return static_cast<bool>(m_pAsset); }

    bool operator==(const AssetHandle<T>& other) const
    {
        return m_pAsset != nullptr && other.m_pAsset != nullptr && m_pAsset == other.m_pAsset;
    }
};
} // namespace aln