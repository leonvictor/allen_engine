#pragma once

#include <memory>

#include "asset.hpp"

namespace aln
{

/// @brief Base class for all asset loaders.
/// Each new asset type will require extending this class to provide customized creation, loading, post-loading, pre-unloading, unloading behaviors
class IAssetLoader
{
    friend class AssetManager;

  public:
    virtual ~IAssetLoader(){};

  private:
    virtual std::shared_ptr<IAsset> Create(AssetGUID id) = 0;
    virtual void Load(std::shared_ptr<IAsset>&) = 0;
    virtual void Unload(std::shared_ptr<IAsset>&) = 0;
    virtual void Initialize(std::shared_ptr<IAsset>&) = 0;
    virtual void Shutdown(std::shared_ptr<IAsset>&) = 0;
};
} // namespace aln