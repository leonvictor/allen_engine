#pragma once

#include "asset_editor_workspace.hpp"
#include "preview_scene_settings.hpp"
#include "preview_scene_settings_window.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <anim/animation_clip.hpp>
#include <common/containers/vector.hpp>

namespace aln
{

class AnimationPlayerComponent;
class SkeletalMeshComponent;

struct EditorAnimationEvent
{
    /// @brief Events below this duration will be fused into a single immediate event
    static constexpr Seconds MinDurationEpsilon = 0.01;

    Seconds m_startTime = 0.0f;
    Seconds m_endTime = 0.0f;

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

  public:
    struct TimelineGraduationsMetrics
    {
        float m_majorGraduationInterval = 0.0f;
        float m_firstMajorGraduation = 0.0f;
        float m_lastMajorGraduation = 0.0f;
        float m_pixelsPerMajorGraduation = 0.0f;
        float m_pixelsPerValue = 0.0f;

        uint32_t m_minorGraduationDivisions = 0;
    };

    struct TimelineRange
    {
        float m_start = 0.0f;
        float m_end = 0.0f;
    };

  private:
    AssetHandle<AnimationClip> m_pAnimationClip;
    Vector<EditorEventTrack*> m_eventTracks;
    EditorEventTrack* m_pSyncTrack = nullptr;
    EditorAnimationEvent* m_pCurrentlyEditedEvent = nullptr;

    State m_state;

    TimelineRange m_viewRange;     // Visible range of values in the timeline editor
    TimelineRange m_playbackRange; // Range of values contained in the edited clip
    bool m_timelinePanningActive = false;

    std::filesystem::path m_compiledAnimationClipPath;
    std::filesystem::path m_statePath;

    bool m_waitingForAssetLoad = true;

    // Preview
    PreviewSceneSettings m_previewSceneSettings;
    Entity* m_pPreviewCameraEntity = nullptr;
    Entity* m_pCharacterEntity = nullptr;
    SkeletalMeshComponent* m_pPreviewCharacterSkeletalMeshComponent = nullptr;
    AnimationPlayerComponent* m_pAnimationPlayerComponent = nullptr;

  private:
    void DrawAnimationEventsEditor();
    void DrawAnimationPlaybackController();

  public:
    // ----- Window lifetime
    void Update(const UpdateContext& context) override;
    void Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile) override;
    void Shutdown() override;
    void Clear();

    PreviewSceneSettings* GetPreviewSceneSettings() override { return &m_previewSceneSettings; }
    void StartEditingScenePreviewSetting(const TypeEditedEventDetails& editingEventDetails) override;
    void EndEditingScenePreviewSetting(const TypeEditedEventDetails& editingEventDetails) override;

    // ----- Compilation
    AnimationClip* Compile();

    // ----- Saving/Loading
    void SaveState(JSON& json) const override;
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override;
};

ALN_ASSET_WORKSPACE_FACTORY(AnimationClip, AnimationClipWorkspace)

} // namespace aln