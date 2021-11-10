#pragma once

#include "asset.hpp"

#include <memory>

namespace aln
{
template <AssetType T>
class AssetHandle
{
    template <AssetType Other>
    friend class AssetHandle;

    friend class AssetLoader;

  private:
    std::shared_ptr<T> m_pAsset;

    mutable size_t* m_pLoadedCount;
    mutable size_t* m_pInitializedCount;

  public:
    ~AssetHandle()
    {
        if (use_count() <= 1)
        {
            delete m_pLoadedCount;
            delete m_pInitializedCount;
        }
        m_pAsset.reset();
    }

    AssetHandle() = default;

    /// @brief Create a new handle to a asset, taking ownership of it
    AssetHandle(std::shared_ptr<T> pAsset) : m_pAsset(std::move(pAsset)), m_pInitializedCount(new size_t(0)), m_pLoadedCount(new size_t(0)) {}

    /// @brief Copy constructs a handled asset, sharing ownership
    /// @todo Only enable for this type, a derived one, or ??? the base IAsset class ???
    template <AssetType Other>
    AssetHandle(const AssetHandle<Other>& handle) : m_pAsset(std::static_pointer_cast<T>(handle.m_pAsset)),
                                                    m_pInitializedCount(handle.m_pInitializedCount),
                                                    m_pLoadedCount(handle.m_pLoadedCount) {}

    /// @brief Move constructor
    template <AssetType Other>
    AssetHandle(AssetHandle<Other>&& handle) : m_pAsset(std::move(std::static_pointer_cast<T>(handle.m_pAsset))),
                                               m_pInitializedCount(std::move(handle.m_pInitializedCount)),
                                               m_pLoadedCount(std::move(handle.m_pLoadedCount)) {}

    size_t use_count() const { return m_pAsset.use_count(); }
    size_t load_count() const { return *m_pLoadedCount; }
    size_t initialized_count() const { return *m_pInitializedCount; }

    T& get() const { return *m_pAsset; }
    T& operator*() const { return *m_pAsset.get(); }
    T* operator->() const { return m_pAsset.get(); }

    operator bool() const { return static_cast<bool>(m_pAsset); }
};
} // namespace aln