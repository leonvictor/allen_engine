#pragma once

#include "asset_id.hpp"
#include "asset_type_id.hpp"

namespace aln
{

class IAsset;

template <class T>
concept AssetType = std::is_convertible<T*, IAsset*>::value;

/// @brief Abstract base class for all assets (textures, meshes, animation...).
class IAsset
{
    friend class IAssetLoader;
    friend class AssetService;
    friend class AssetRequest;

  private:
    AssetID m_id;

  public:
    virtual ~IAsset(){}

    inline const AssetID& GetID() const { return m_id; }
    virtual AssetTypeID GetAssetTypeID() const = 0;

    inline bool operator==(const IAsset& other) const { return m_id == other.m_id; }
};
} // namespace aln

/// @brief Use this macro in each asset type to define file extension
/// @todo Accept a human-readable name as well
/// @todo Register the known types somewhere
#define ALN_REGISTER_ASSET_TYPE(assetTypeExtension)                                                 \
  public:                                                                                           \
    static AssetTypeID GetStaticAssetTypeID() { return AssetTypeID(assetTypeExtension); }           \
    virtual AssetTypeID GetAssetTypeID() const override { return AssetTypeID(assetTypeExtension); } \
                                                                                                    \
  private: