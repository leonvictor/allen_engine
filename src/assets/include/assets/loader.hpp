#pragma once

#include <memory>

#include "asset.hpp"
#include "handle.hpp"
#include "record.hpp"

namespace aln
{
/// TODO: Hide from clients
class AssetLoader
{
    friend class AssetManager;

  public:
    virtual ~AssetLoader(){};

  protected:
    virtual IAsset* Create(AssetGUID id) = 0;
    virtual bool Load(AssetRecord* pAsset) = 0;
    virtual void Unload(AssetRecord* pAsset) = 0;
};

/// @brief Base class for all asset loaders.
/// Each new asset type will require extending this class to provide customized creation, loading, post-loading, pre-unloading, unloading behaviors
template <AssetType T>
class IAssetLoader : public AssetLoader
{
  public:
    virtual ~IAssetLoader(){};

  protected:
    /// @todo Creation can be delayed until the asset is loaded for the first time
    IAsset* Create(AssetGUID id) final override { return aln::New<T>(id); }
    virtual bool Load(AssetRecord* pRecord) = 0;
    virtual void Unload(AssetRecord* pRecord) = 0;
};
} // namespace aln