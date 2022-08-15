#pragma once

#include "asset.hpp"
#include "record.hpp"

#include <common/memory.hpp>
#include <memory>

namespace aln
{

class IAssetHandle
{
    friend class AssetLoader;
    friend class AssetManager;

  private:
    AssetID m_assetID;
    AssetStatus GetStatus() const { return (m_pAssetRecord != nullptr) ? m_pAssetRecord->GetStatus() : AssetStatus::Unloaded; }

  public:
    // TODO: shouldn't be public
    const AssetRecord* m_pAssetRecord = nullptr;

    IAssetHandle() = default;
    IAssetHandle(AssetID assetID) : m_assetID(assetID) {}
    IAssetHandle(std::string assetPath) : m_assetID(assetPath) {}

    // ------------------------------
    // IDs
    // ------------------------------
    inline const AssetID& GetAssetID() const { return m_assetID; }
    inline const AssetTypeID& GetAssetTypeID() const { return m_assetID.GetAssetTypeID(); }
    inline const std::string& GetAssetPath() const { return m_assetID.GetAssetPath(); }

    /// @todo: Remove when dependencies are correctly loaded from asset metadata
    const AssetRecord* GetRecord() const { return m_pAssetRecord; }

    // ------------------------------
    // Status management
    // ------------------------------
    inline bool IsLoaded() const { return GetStatus() == AssetStatus::Loaded; }
    inline bool IsUnloaded() const { return GetStatus() == AssetStatus::Unloaded; }
    operator bool() const { return m_pAssetRecord != nullptr; }
};

template <AssetType T>
class AssetHandle : public IAssetHandle
{
    template <AssetType Other>
    friend class AssetHandle;

    friend class AssetLoader;
    friend class AssetManager;

  public:
    AssetHandle() = default;
    AssetHandle(AssetID assetID) : IAssetHandle(assetID)
    {
        assert(GetAssetTypeID() == T::GetStaticAssetTypeID());
    }
    AssetHandle(std::string assetPath) : IAssetHandle(assetPath)
    {
        assert(GetAssetTypeID() == T::GetStaticAssetTypeID());
    }

    // ------------------------------
    // Underlying asset access
    // ------------------------------
    const T* get() const { return (T*) m_pAssetRecord->GetAsset(); }
    const T* operator->() const { return (T*) m_pAssetRecord->GetAsset(); }

    bool operator==(const AssetHandle<T>& other) const
    {
        return m_pAssetRecord != nullptr && other.m_pAssetRecord != nullptr && m_pAssetRecord == other.m_pAssetRecord;
    }
};
} // namespace aln