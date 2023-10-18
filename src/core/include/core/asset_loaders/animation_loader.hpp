#pragma once

#include <anim/animation_clip.hpp>
#include <assets/loader.hpp>
#include <graphics/render_engine.hpp>

namespace aln
{

class AnimationLoader : public IAssetLoader
{
  private:
    RenderEngine* m_pRenderEngine;

  public:
    AnimationLoader(RenderEngine* pDevice)
    {
        m_pRenderEngine = pDevice;
    }

    bool Load(RequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override
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

        pAnim->m_frameCount = pAnim->m_tracks[0].m_transforms.size();

        archive >> pAnim->m_rootMotionTrack;

        // TMP: Explicitely creates a default sync track
        pAnim->m_syncTrack = SyncTrack::Default;

        // TODO: Add a dependency on the skeleton, and ensure it is loaded correctly

        pRecord->SetAsset(pAnim);
        return true;
    }
};

} // namespace aln