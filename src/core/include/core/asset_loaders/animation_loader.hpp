#pragma once

#include <assets/loader.hpp>

#include <anim/animation_clip.hpp>

#include <memory>
#include <vector>

namespace aln
{

class AnimationLoader : public IAssetLoader
{
  private:
    vkg::Device* m_pDevice;

  public:
    AnimationLoader(vkg::Device* pDevice)
    {
        m_pDevice = pDevice;
    }

    bool Load(AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());

        AnimationClip* pAnim = aln::New<AnimationClip>();

        archive >> pAnim->m_duration;
        archive >> pAnim->m_framesPerSecond;

        size_t trackCount;
        archive >> trackCount;

        pAnim->m_tracks.reserve(trackCount);
        for (auto trackIndex = 0; trackIndex < trackCount; ++trackIndex)
        {
            auto& track = pAnim->m_tracks.emplace_back();
            archive >> track.m_transforms;
        }

        archive >> pAnim->m_rootMotionTrack;

        // TODO: Add a dependency on the skeleton, and ensure it is loaded correctly

        pRecord->SetAsset(pAnim);
        return true;
    }
};

} // namespace aln