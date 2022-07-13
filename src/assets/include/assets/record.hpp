#pragma once

#include "asset.hpp"

namespace aln
{
/// @brief Record kept by the asset manager service, which counts the number of references.
// A record for an asset should be unique.
class AssetRecord
{
    friend class AssetManager;

  private:
    IAsset* m_pAsset = nullptr;
    uint32_t m_referenceCount = 0;

    void AddReference() { m_referenceCount++; }
    void RemoveReference() { m_referenceCount--; }
    uint32_t GetReferenceCount() { return m_referenceCount; }

  public:
    AssetGUID GetID()
    {
        assert(m_pAsset != nullptr);
        return m_pAsset->GetID();
    }

    template <typename T>
    T* GetAsset()
    {
        assert(m_pAsset != nullptr);
        return static_cast<T*>(m_pAsset);
    }

    IAsset* GetAsset()
    {
        assert(m_pAsset != nullptr);
        return m_pAsset;
    }

    operator bool() const { return m_pAsset != nullptr; }

    bool operator==(const AssetRecord& other) const
    {
        return m_pAsset != nullptr && other.m_pAsset != nullptr && m_pAsset == other.m_pAsset;
    }
};
} // namespace aln