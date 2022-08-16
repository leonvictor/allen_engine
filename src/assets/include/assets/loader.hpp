#pragma once

#include <assert.h>
#include <memory>

#include "asset_system/asset_system.hpp"

#include "asset.hpp"
#include "handle.hpp"
#include "record.hpp"

namespace aln
{
/// TODO: Hide from clients
/// @todo: Swap class names
class AssetLoader
{
    friend class AssetService;
    friend class AssetRequest;

  private:
    bool LoadAsset(AssetRecord* pRecord)
    {
        assets::AssetFile file;
        auto loaded = assets::LoadBinaryFile(pRecord->GetAssetPath(), file);
        if (!loaded)
        {
            // TODO: Properly handle load failure
            assert(0);
            return false;
        }

        for (auto& dependency : file.m_dependencies)
        {
            pRecord->AddDependency(dependency);
        }

        return Load(pRecord, file);
    }
    void UnloadAsset(AssetRecord* pAssetRecord) { Unload(pAssetRecord); }
    virtual void InstallAsset(const AssetID& assetID, AssetRecord* pAssetRecord, const std::vector<IAssetHandle>& dependencies)
    {
        assert(pAssetRecord->IsUnloaded());
        assert(assetID.IsValid());

        pAssetRecord->m_pAsset->m_id = assetID;
        InstallDependencies(pAssetRecord, dependencies);
    }

  protected:
    virtual bool Load(AssetRecord* pAsset, const assets::AssetFile& file) = 0;
    virtual void Unload(AssetRecord* pAsset) = 0;
    virtual void InstallDependencies(AssetRecord* pAssetRecord, const std::vector<IAssetHandle>& dependencies) {}

  public:
    virtual ~AssetLoader(){};
};

/// @brief Base class for all asset loaders.
/// Each new asset type will require extending this class to provide customized creation, loading, post-loading, pre-unloading, unloading behaviors
template <AssetType T>
class IAssetLoader : public AssetLoader
{
  protected:
    virtual bool Load(AssetRecord* pRecord, const assets::AssetFile& file) = 0;
    virtual void Unload(AssetRecord* pRecord) = 0;
    virtual void InstallDependencies(AssetRecord* pAssetRecord, const std::vector<IAssetHandle>& dependencies) {}

    const AssetRecord* GetDependencyRecord(const std::vector<IAssetHandle>& dependencies, const AssetID& dependencyID)
    {
        for (auto& dependencyHandle : dependencies)
        {
            if (dependencyHandle.GetAssetID() == dependencyID)
            {
                return dependencyHandle.GetRecord();
            }
            assert(0);
        }
    }
};
} // namespace aln