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
class IAssetLoader
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

    void UnloadAsset(AssetRecord* pRecord)
    {
        assert(pRecord->IsLoaded());
        Unload(pRecord);
    }

    virtual void InstallAsset(const AssetID& assetID, AssetRecord* pRecord, const std::vector<IAssetHandle>& dependencies)
    {
        assert(pRecord->IsUnloaded());
        assert(assetID.IsValid());

        pRecord->m_pAsset->m_id = assetID;
        InstallDependencies(pRecord, dependencies);
    }

  protected:
    virtual bool Load(AssetRecord* pRecord, const assets::AssetFile& file) = 0;

    virtual void Unload(AssetRecord* pRecord)
    {
        auto pAsset = pRecord->GetAsset();
        aln::Delete(pAsset);
        pRecord->m_pAsset = nullptr;
    };

    virtual void InstallDependencies(AssetRecord* pRecord, const std::vector<IAssetHandle>& dependencies) {}

    const AssetRecord* GetDependencyRecord(const std::vector<IAssetHandle>& dependencies, const AssetID& dependencyID)
    {
        for (auto& dependencyHandle : dependencies)
        {
            if (dependencyHandle.GetAssetID() == dependencyID)
            {
                return dependencyHandle.GetRecord();
            }
        }
        assert(0); // Not reachable
        return nullptr;
    }

  public:
    virtual ~IAssetLoader(){};
};
} // namespace aln