#pragma once

#include "asset_editor_workspace.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <anim/animation_clip.hpp>
#include <common/containers/vector.hpp>

namespace aln
{

struct EditorAnimationEvent
{
    /// @brief Events below this duration will be fused into a single immediate event
    static constexpr float MinDurationEpsilon = 0.01;

    float m_startTime = 0.0f;
    float m_endTime = 0.0f;

    bool IsImmediate() const { return Maths::IsNearEqual(m_startTime, m_endTime, MinDurationEpsilon); }
    bool IsDurable() const { return !Maths::IsNearEqual(m_startTime, m_endTime, MinDurationEpsilon); }
};

struct EditorEventTrack
{
    std::string m_name;
    Vector<EditorAnimationEvent*> m_events;
};

class AnimationClipWorkspace : public IAssetWorkspace
{
    struct State
    {
        float m_initialStartTime = 0.0f;
        float m_initialEndTime = 0.0f;
    };

  private:
    AssetHandle<AnimationClip> m_pAnimationClip;
    Vector<EditorEventTrack*> m_eventTracks;
    EditorEventTrack* m_pSyncTrack = nullptr;
    EditorAnimationEvent* m_pCurrentlyEditedEvent = nullptr;
    
    State m_state;

    std::filesystem::path m_compiledAnimationClipPath;
    std::filesystem::path m_statePath;

    bool m_waitingForAssetLoad = true;

    // Sequencer state
    float m_animationTime = 0.0f;

  private:
    void DrawAnimationPreview();
    void DrawAnimationEventsEditor();

  public:
    // ----- Window lifetime

    void Update(const UpdateContext& context) override;
    virtual void Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile) override;
    virtual void Shutdown() override;
    void Clear();

    // ----- Saving/Loading

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