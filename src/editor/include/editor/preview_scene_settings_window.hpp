#pragma once

#include "asset_editor_workspace.hpp"
#include "editor_window.hpp"
#include "preview_scene_settings.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <assets/asset_id.hpp>
#include <common/containers/vector.hpp>
#include <core/skeletal_mesh.hpp>

#include <imgui.h>

namespace aln
{

/// @brief Settings for a preview scene, allowing to pick de displayed mesh for example
/// @todo Track a "currently active" preview scene
/// @todo Allow different workspace to specialize some of the options (
class PreviewSceneSettingsWindow : public IEditorWindow
{
  private:
    ReflectedTypeEditor m_reflectedTypeEditor;

    UUID m_settingEditingStartedEventID;
    UUID m_settingEditingCompletedEventID;

    IAssetWorkspace* m_pAssetWorkspace = nullptr;

  private:
    void StartSettingEditing(const TypeEditedEventDetails& eventDetails)
    {
        // Forward to focused window
        assert(m_pAssetWorkspace != nullptr);
        m_pAssetWorkspace->StartEditingScenePreviewSetting(eventDetails);
    }
    void EndSettingEditing(const TypeEditedEventDetails& eventDetails)
    {
        // Forward to focused window
        assert(m_pAssetWorkspace != nullptr);
        m_pAssetWorkspace->EndEditingScenePreviewSetting(eventDetails);
    }

  public:
    void Initialize(EditorWindowContext* pEditorWindowContext) override
    {
        IEditorWindow::Initialize(pEditorWindowContext);

        m_reflectedTypeEditor.Initialize();

        m_settingEditingStartedEventID = m_reflectedTypeEditor.OnTypeEditingStarted().BindListener([this](const TypeEditedEventDetails& eventDetails)
            { StartSettingEditing(eventDetails); });
        m_settingEditingCompletedEventID = m_reflectedTypeEditor.OnTypeEditingCompleted().BindListener([this](const TypeEditedEventDetails& eventDetails)
            { EndSettingEditing(eventDetails); });
    }

    void Shutdown() override
    {
        m_reflectedTypeEditor.OnTypeEditingStarted().UnbindListener(m_settingEditingStartedEventID);
        m_reflectedTypeEditor.OnTypeEditingCompleted().UnbindListener(m_settingEditingCompletedEventID);
        
        m_reflectedTypeEditor.Shutdown();

        IEditorWindow::Shutdown();
    }

    void Update(const UpdateContext& context) override
    {
        m_pAssetWorkspace = GetFocusedWorkspace();
        if (m_pAssetWorkspace == nullptr)
        {
            return;
        }

        auto pPreviewSceneSettings = m_pAssetWorkspace->GetPreviewSceneSettings();
        if (pPreviewSceneSettings == nullptr)
        {
            return;
        }

        /// @todo Add an icon
        ImGui::Begin("Preview Scene Settings");

        auto pTypeInfo = pPreviewSceneSettings->GetTypeInfo();
        m_reflectedTypeEditor.Draw(pTypeInfo, pPreviewSceneSettings);

        ImGui::End();
    }

    virtual void LoadState(JSON& json, const TypeRegistryService* pTypeRegistryService) override{};
    virtual void SaveState(JSON& json) const override{};
};
} // namespace aln