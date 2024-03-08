#pragma once

#include "asset.hpp"
#include "asset_archive_header.hpp"
#include "handle.hpp"
#include "record.hpp"
#include "request_context.hpp"

#include <common/serialization/binary_archive.hpp>
#include <graphics/command_buffer.hpp>

#include <assert.h>
#include <memory>

namespace aln
{

/// TODO: Hide from clients
class IAssetLoader
{
    friend class AssetService;
    friend class AssetRequest;

  private:
    // Concrete loading functions called by the asset service
    bool LoadAsset(AssetRequestContext& ctx, AssetRecord* pRecord)
    {
        assert(pRecord->IsUnloaded());

        // TODO: Is this the right place for loading *from disk* ?
        // The full archive could be dumped to a Vector<byte> earlier (and it would possibly be faster)
        auto archive = BinaryFileArchive(pRecord->GetAssetPath(), IBinaryArchive::IOMode::Read);
        if (!archive.IsValid())
        {
            // TODO: Properly handle load failure
            assert(0);
            return false;
        }

        AssetArchiveHeader header;
        Vector<std::byte> dataStream;

        archive >> header;
        archive >> dataStream;

        for (auto& dependency : header.GetDependencies())
        {
            pRecord->AddDependency(dependency);
        }

        auto dataArchive = BinaryMemoryArchive(dataStream, IBinaryArchive::IOMode::Read);

        return Load(ctx, pRecord, dataArchive);
    }

    void UnloadAsset(AssetRecord* pRecord)
    {
        assert(pRecord->IsLoaded());

        Unload(pRecord);

        auto pAsset = pRecord->GetAsset();
        aln::Delete(pAsset);
        pRecord->m_pAsset = nullptr;
    }

    void InstallAsset(const AssetID& assetID, AssetRecord* pRecord, const Vector<IAssetHandle>& dependencies)
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->m_pAsset != nullptr);
        assert(assetID.IsValid());

        pRecord->m_pAsset->m_id = assetID;
        InstallDependencies(pRecord, dependencies);
    }

  protected:
    // Virtual loading functions, overload in specialized loader classes to implement asset-specific behavior
    virtual bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) = 0;
    virtual void Unload(AssetRecord* pRecord){};
    virtual void InstallDependencies(AssetRecord* pRecord, const Vector<IAssetHandle>& dependencies) {}

    const AssetRecord* GetDependencyRecord(const Vector<IAssetHandle>& dependencies, size_t dependencyIndex)
    {
        assert(dependencyIndex >= 0 && dependencyIndex < dependencies.size());
        return dependencies[dependencyIndex].GetRecord();
    }

    void UpdateDependencyRecord(IAssetHandle handle, const AssetRecord* pRecord)
    {
        handle.m_pAssetRecord = pRecord;
    }

    const AssetRecord* GetDependencyRecord(const Vector<IAssetHandle>& dependencies, const AssetID& dependencyID)
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