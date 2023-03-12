#pragma once

#include "asset.hpp"
#include "record.hpp"

#include <common/memory.hpp>
#include <memory>

#include <reflection/type_info.hpp>

namespace aln
{

class IAssetHandle
{
    friend class IAssetLoader;
    friend class AssetService;

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
    inline bool HasFailedLoading() const { return GetStatus() == AssetStatus::LoadingFailed; }
    operator bool() const { return m_pAssetRecord != nullptr; }

    // --------- Serialization
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_assetID;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_assetID;
    }
};

template <AssetType T>
class AssetHandle : public IAssetHandle
{
    template <AssetType Other>
    friend class AssetHandle;

    friend class IAssetLoader;
    friend class AssetService;

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

namespace reflect
{
template <>
struct TypeInfoResolver<AssetHandle<IAsset>>
{
    static const TypeInfo* Get()
    {
        static PrimitiveTypeInfo typeInfo;
        if (!typeInfo.IsValid())
        {
            typeInfo.m_name = "AssetHandle";
            typeInfo.m_typeID = StringID(typeInfo.m_name);
            typeInfo.m_alignment = alignof(AssetHandle<IAsset>);
            typeInfo.m_size = sizeof(AssetHandle<IAsset>);
            typeInfo.m_createType = []()
            { return aln::New<AssetHandle<IAsset>>(); };
            typeInfo.m_createTypeInPlace = [](void* pMemory)
            { return aln::PlacementNew<AssetHandle<IAsset>>(pMemory); };
            typeInfo.m_serialize = [](BinaryMemoryArchive& archive, const void* pTypeInstance)
            { archive << *((IAssetHandle*) pTypeInstance); };
            typeInfo.m_deserialize = [](BinaryMemoryArchive& archive, void* pTypeInstance)
            { archive >> *((IAssetHandle*) pTypeInstance); };

            TypeInfo::RegisterTypeInfo(&typeInfo);
        }

        return &typeInfo;
    }
};

template <typename T>
struct TypeInfoResolver<AssetHandle<T>>
{
    static const TypeInfo* Get()
    {
        return TypeInfoResolver<AssetHandle<IAsset>>::Get();
    }
};
} // namespace reflect
} // namespace aln