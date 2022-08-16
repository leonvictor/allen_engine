#pragma once

#include <assets/asset_system/animation_clip_asset.hpp>
#include <assets/asset_system/asset_system.hpp>
#include <assets/loader.hpp>

#include <anim/animation_clip.hpp>

#include <memory>
#include <vector>

namespace aln
{

class AnimationLoader : public IAssetLoader
{
  private:
    std::shared_ptr<vkg::Device> m_pDevice;

  public:
    AnimationLoader(std::shared_ptr<vkg::Device> pDevice)
    {
        m_pDevice = pDevice;
    }

    bool Load(AssetRecord* pRecord, const assets::AssetFile& file) override
    {
        assert(pRecord->IsUnloaded());
        assert(file.m_assetTypeID == AnimationClip::GetStaticAssetTypeID());

        AnimationClip* pAnim = aln::New<AnimationClip>();

        auto info = assets::ReadAnimationClipInfo(&file);
        pAnim->m_ticksPerSecond = info.framesPerSecond;
        pAnim->m_duration = (float) info.duration;

        // TODO: Stream directly to the tracks
        std::vector<float> buffer;
        buffer.resize(info.binaryBufferSize / sizeof(float));
        assets::UnpackAnimationClip(&info, file.m_binary, buffer);

        float* dataPtr = buffer.data();
        for (auto& trackInfo : info.tracks)
        {
            auto& track = pAnim->m_tracks.emplace_back();
            track.m_boneName = trackInfo.boneName;

            track.m_translationKeys.resize(trackInfo.numTranslationKeys);
            memcpy(track.m_translationKeys.data(), dataPtr, (trackInfo.numTranslationKeys * 4) * sizeof(float));
            dataPtr += (trackInfo.numTranslationKeys * 4);

            track.m_rotationKeys.resize(trackInfo.numRotationKeys);
            memcpy(track.m_rotationKeys.data(), dataPtr, (trackInfo.numRotationKeys * 5) * sizeof(float));
            dataPtr += (trackInfo.numRotationKeys * 5);

            track.m_scaleKeys.resize(trackInfo.numScaleKeys);
            memcpy(track.m_scaleKeys.data(), dataPtr, (trackInfo.numScaleKeys * 4) * sizeof(float));
            dataPtr += (trackInfo.numScaleKeys * 4);
        }

        // TODO: Add a dependency on the skeleton, and ensure it is loaded correctly
        pRecord->SetAsset(pAnim);
        return true;
    }
};

} // namespace aln