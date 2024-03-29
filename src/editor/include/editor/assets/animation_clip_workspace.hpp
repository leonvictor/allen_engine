#pragma once

#include "asset_editor_workspace.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <anim/animation_clip.hpp>
#include <common/containers/vector.hpp>

namespace aln
{

// TODO: Naming...
class AnimationEventEditor
{
    float m_startTime = 0.0f;
    float m_duration = 0.0f;
};

struct EventTrack
{
    std::string m_name;
    Vector<AnimationEventEditor*> m_events;
};

class AnimationClipWorkspace : public IAssetWorkspace
{
  private:
    AssetHandle<AnimationClip> m_pAnimationClip;
    Vector<EventTrack*> m_eventTracks;
    EventTrack* m_pSyncTrack = nullptr;

    std::filesystem::path m_compiledAnimationClipPath;
    std::filesystem::path m_statePath;

    bool m_waitingForAssetLoad = true;

    // Sequencer state
    float m_animationTime = 2.5f; // TODO

  public:
    // ----------- Window lifetime

    void Update(const UpdateContext& context) override;
    virtual void Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile) override;
    virtual void Shutdown() override;
    void Clear();

    // -------------- Saving/Loading

    AnimationClip* Compile();

    virtual void SaveState(JSON& json) const override
    {
        // TODO
    }
    virtual void LoadState(JSON& json, const TypeRegistryService* pTypeRegistryService) override
    {
        // TODO
    }
};

ALN_ASSET_WORKSPACE_FACTORY(AnimationClip, AnimationClipWorkspace)

} // namespace aln