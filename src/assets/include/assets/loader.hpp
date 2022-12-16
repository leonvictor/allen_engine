#pragma once

#include <assert.h>
#include <memory>

#include <common/serialization/binary_archive.hpp>

#include "asset.hpp"
#include "asset_archive_header.hpp"
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
    // Concrete loading functions called by the asset service
    bool LoadAsset(AssetRecord* pRecord)
    {
        assert(pRecord->IsUnloaded());

        // TODO: Is this the right place for loading *from disk* ?
        // The full archive could be dumped to a std::vector<byte> earlier (and it would possibly be faster)
        auto archive = BinaryFileArchive(pRecord->GetAssetPath(), IBinaryArchive::IOMode::Read);
        if (!archive.IsValid())
        {
            // TODO: Properly handle load failure
            assert(0);
            return false;
        }

        AssetArchiveHeader header;
        archive >> header;

        std::vector<std::byte> data;
        archive >> data;

        for (auto& dependency : header.GetDependencies())
        {
            pRecord->AddDependency(dependency);
        }

        auto dataArchive = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Read);
        return Load(pRecord, dataArchive);
    }

    void UnloadAsset(AssetRecord* pRecord)
    {
        assert(pRecord->IsLoaded());

        Unload(pRecord);

        auto pAsset = pRecord->GetAsset();
        aln::Delete(pAsset);
        pRecord->m_pAsset = nullptr;
    }

    void InstallAsset(const AssetID& assetID, AssetRecord* pRecord, const std::vector<IAssetHandle>& dependencies)
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->m_pAsset != nullptr);
        assert(assetID.IsValid());

        pRecord->m_pAsset->m_id = assetID;
        InstallDependencies(pRecord, dependencies);
    }

  protected:
    // Virtual loading functions, overload in specialized loader classes to implement asset-specific behavior
    virtual bool Load(AssetRecord* pRecord, BinaryMemoryArchive& archive) = 0;
    virtual void Unload(AssetRecord* pRecord){};
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