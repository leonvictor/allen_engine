#pragma once

#include "asset.hpp"
#include "asset_id.hpp"
#include "status.hpp"

#include <common/containers/vector.hpp>

namespace aln
{
/// @brief Record kept by the asset manager service, which counts the number of references.
// A record for an asset should be unique.
class AssetRecord
{
    friend class AssetService;
    friend class AssetRequest;
    friend class IAssetLoader;

  private:
    AssetID m_assetID;
    IAsset* m_pAsset = nullptr;

    Vector<AssetID> m_dependencies;

    // Runtime state
    AssetStatus m_status = AssetStatus::Unloaded;
    uint32_t m_referenceCount = 0;

    void AddReference() { m_referenceCount++; }
    void RemoveReference() { m_referenceCount--; }
    uint32_t GetReferenceCount() { return m_referenceCount; }

  public:
    AssetRecord(AssetID assetID) : m_assetID(assetID) {}

    // ------------------------------
    // IDs
    // ------------------------------
    const AssetID& GetAssetID() const { return m_assetID; }
    const AssetTypeID& GetAssetTypeID() const { return m_assetID.GetAssetTypeID(); }
    const std::string& GetAssetPath() const { return m_assetID.GetAssetPath(); }

    // ------------------------------
    // Underlying asset access
    // ------------------------------
    template <typename T>
    T* GetAsset()
    {
        assert(m_pAsset != nullptr);
        return static_cast<T*>(m_pAsset);
    }

    IAsset* GetAsset() const
    {
        assert(m_pAsset != nullptr);
        return m_pAsset;
    }

    void SetAsset(IAsset* pAsset) { m_pAsset = pAsset; }

    // ------------------------------
    // Status management
    // ------------------------------
    AssetStatus GetStatus() const { return m_status; }
    inline bool IsLoaded() const { return m_status == AssetStatus::Loaded; }
    inline bool IsUnloaded() const { return m_status == AssetStatus::Unloaded; }

    // ------------------------------
    // Dependencies
    // ------------------------------
    bool HasDependencies() const { return !m_dependencies.empty(); }
    const Vector<AssetID>& GetDependencies() const { return m_dependencies; }
    /// @todo: Remove when dependencies are correctly loaded from asset metadata
    void AddDependency(const AssetID& assetID) { m_dependencies.push_back(assetID); }

    operator bool() const { return m_pAsset != nullptr; }
    bool operator==(const AssetRecord& other) const
    {
        return m_pAsset != nullptr && other.m_pAsset != nullptr && m_pAsset == other.m_pAsset;
    }
};
} // namespace aln