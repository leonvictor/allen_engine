#pragma once

#include <assets/asset_system/asset_system.hpp>
// #include <assets/asset_system/skeleton_asset.hpp>
#include <assets/loader.hpp>

#include <anim/skeleton.hpp>

#include <memory>
#include <vector>

namespace aln
{

class SkeletonLoader : public IAssetLoader<Skeleton>
{
  private:
    bool Load(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsUnloaded());
        auto pSkeleton = AssetHandle<Skeleton>(pAsset);

        assets::AssetFile file;
        auto loaded = assets::LoadBinaryFile(pSkeleton->GetID().GetAssetPath(), file);
        if (!loaded)
        {
            // TODO: Actually handle
            // return false;
            return true;
        }

        // assert(file.type == assets::EAssetType::Skeleton);

        // auto info = assets::ReadSkeletonInfo(&file);

        // TODO: Stream directly to the tracks
        // std::vector<float> buffer;
        // buffer.resize(info.bufferSize);
        // assets::UnpackSkeleton(&info, file.binary, buffer);

        // size_t index = 0;
        // for (auto& trackInfo : info.tracks)
        // {
        //     auto& track = pAnim->m_tracks.emplace_back();
        //     track.m_boneName = trackInfo.boneName;

        //     track.m_keys.resize(trackInfo.numKeys);
        //     memcpy(track.m_keys.data(), buffer.data() + index, sizeof(TrackKey) * trackInfo.numKeys);

        //     index += trackInfo.numKeys * (1 + 4 + 3 + 3);
        // }

        return true;
    }

    void Unload(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsLoaded());
        auto pSkeleton = AssetHandle<Skeleton>(pAsset);
        // TODO
    }

    void Initialize(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsLoaded());
        auto pSkeleton = AssetHandle<Skeleton>(pAsset);
        // TODO
    }

    void Shutdown(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsInitialized());
        auto pSkeleton = AssetHandle<Skeleton>(pAsset);
        // TODO
    }
};

} // namespace aln