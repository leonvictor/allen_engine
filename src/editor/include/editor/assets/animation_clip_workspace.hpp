#pragma once

#include "asset_editor_workspace.hpp"
#include "preview_scene_settings_window.hpp"
#include "preview_scene_settings.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <anim/animation_clip.hpp>
#include <common/containers/vector.hpp>
#include <core/components/skeletal_mesh_component.hpp>
#include <entities/world_entity.hpp>

namespace aln
{

class AnimationPlayerComponent;

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

    // Preview
    PreviewSceneSettings m_previewSceneSettings;
    Entity* m_pPreviewCameraEntity = nullptr;
    Entity* m_pCharacterEntity = nullptr;
    SkeletalMeshComponent* m_pPreviewCharacterSkeletalMeshComponent = nullptr;
    AnimationPlayerComponent* m_pAnimationPlayerComponent = nullptr;

    // Sequencer state
    float m_animationTime = 0.0f;

  private:
    void DrawAnimationPreview();
    void DrawAnimationEventsEditor();

  public:
    // ----- Window lifetime
    void Update(const UpdateContext& context) override;
    void Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile) override;
    void Shutdown() override;
    void Clear();

    PreviewSceneSettings* GetPreviewSceneSettings() override { return &m_previewSceneSettings; }
    
    void StartEditingScenePreviewSetting(const TypeEditedEventDetails& editingEventDetails) override
    {
        assert(HasPreviewWorld());

        if (editingEventDetails.m_pEditedMember == &m_previewSceneSettings.m_pSkeletalMesh)
        {
            m_pPreviewWorld->StartComponentEditing(m_pPreviewCharacterSkeletalMeshComponent);
        }
    }

    void EndEditingScenePreviewSetting(const TypeEditedEventDetails& editingEventDetails) override
    {
        assert(HasPreviewWorld());

        if (editingEventDetails.m_pEditedMember == &m_previewSceneSettings.m_pSkeletalMesh)
        {
            m_pPreviewCharacterSkeletalMeshComponent->SetMesh(m_previewSceneSettings.m_pSkeletalMesh.GetAssetID());
            m_pPreviewWorld->EndComponentEditing(m_pPreviewCharacterSkeletalMeshComponent);
        }
    }

    // ----- Compilation
    AnimationClip* Compile();

    // ----- Saving/Loading
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