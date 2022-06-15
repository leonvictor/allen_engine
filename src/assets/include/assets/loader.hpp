#pragma once

#include <memory>

#include "asset.hpp"
#include "handle.hpp"

namespace aln
{
/// TODO: Hide from clients
class AssetLoader
{
    friend class AssetManager;

  public:
    virtual ~AssetLoader(){};

  private:
    bool LoadAsset(const AssetHandle<IAsset>& pHandle)
    {
        if (pHandle->IsLoaded())
        {
            (*pHandle.m_pLoadedCount)++;
            return true;
        }

        if (Load(pHandle))
        {
            /// TODO: Status setting could happen in the base Asset class (same way as here)
            pHandle->m_status = IAsset::Status::Loaded;
            (*pHandle.m_pLoadedCount)++;
            return true;
        }

        return false;
    }

    void UnloadAsset(const AssetHandle<IAsset>& handle)
    {
        if (handle.load_count() == 1) // Only unload if this is the last one
        {
            assert(handle->IsLoaded());
            Unload(handle);
            handle->m_status = IAsset::Status::Unloaded;
        }
        (*handle.m_pLoadedCount)--;
    }

  protected:
    virtual AssetHandle<IAsset> Create(AssetGUID id) = 0;
    virtual bool Load(const AssetHandle<IAsset>&) = 0;
    virtual void Unload(const AssetHandle<IAsset>&) = 0;
};

/// @brief Base class for all asset loaders.
/// Each new asset type will require extending this class to provide customized creation, loading, post-loading, pre-unloading, unloading behaviors
template <AssetType T>
class IAssetLoader : public AssetLoader
{
  public:
    virtual ~IAssetLoader(){};

  protected:
    virtual AssetHandle<IAsset> Create(AssetGUID id)
    {
        return AssetHandle<IAsset>(std::make_shared<T>(id));
    }

    virtual bool Load(const AssetHandle<IAsset>&) = 0;
    virtual void Unload(const AssetHandle<IAsset>&) = 0;
};
} // namespace aln