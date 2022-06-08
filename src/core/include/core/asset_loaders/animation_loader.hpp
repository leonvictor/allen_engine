#pragma once

#include <assets/asset_system/animation_clip_asset.hpp>
#include <assets/asset_system/asset_system.hpp>
#include <assets/loader.hpp>

#include <anim/animation_clip.hpp>

#include <memory>
#include <vector>

namespace aln
{

class AnimationLoader : public IAssetLoader<AnimationClip>
{
  private:
    std::shared_ptr<vkg::Device> m_pDevice;

  public:
    AnimationLoader(std::shared_ptr<vkg::Device> pDevice)
    {
        m_pDevice = pDevice;
    }

    bool Load(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsUnloaded());
        auto pAnim = AssetHandle<AnimationClip>(pAsset);

        assets::AssetFile file;
        auto loaded = assets::LoadBinaryFile(pAnim->GetID().GetAssetPath(), file);
        if (!loaded)
        {
            return false;
        }

        assert(file.type == assets::EAssetType::Animation);

        auto info = assets::ReadAnimationClipInfo(&file);

        // TODO: Stream directly to the tracks
        std::vector<float> buffer;
        buffer.resize(info.bufferSize);
        assets::UnpackAnimationClip(&info, file.binary, buffer);

        size_t index = 0;
        for (auto& trackInfo : info.tracks)
        {
            auto& track = pAnim->m_tracks.emplace_back();
            track.m_boneName = trackInfo.boneName;

            track.m_keys.resize(trackInfo.numKeys);
            memcpy(track.m_keys.data(), buffer.data() + index, sizeof(TrackKey) * trackInfo.numKeys);

            index += trackInfo.numKeys * (1 + 4 + 3 + 3);
        }

        return true;
    }

    void Unload(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsLoaded());
        auto pAnim = AssetHandle<AnimationClip>(pAsset);
        // TODO
    }

    void Initialize(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsLoaded());
        auto pAnim = AssetHandle<AnimationClip>(pAsset);
        // TODO
    }

    void Shutdown(const AssetHandle<IAsset>& pAsset) override
    {
        assert(pAsset->IsInitialized());
        auto pAnim = AssetHandle<AnimationClip>(pAsset);
        // TODO
    }
};

} // namespace aln