#pragma once

#include "editor_window.hpp"
#include "reflected_types/reflected_type_editor.hpp"

namespace aln
{
class EntityInspector : public IEditorWindow
{
    ReflectedTypeEditor m_entityComponentsSystemInspector;
    UUID m_entityStartedEditingEventID;

    // TODO: Move to the quaternion widget directly
    glm::vec3 m_currentEulerRotation; // Inspector's rotation is stored separately to avoid going back and forth between quat and euler
    bool m_uniformScale = true;
    Entity* m_pEntity;

    const TypeRegistryService* m_pTypeRegistryService = nullptr;

  public:
    void Initialize(EditorWindowContext* pEditorWindowContext) override;
    void Shutdown() override;
    virtual void Update(const UpdateContext& context);

    void BeginComponentEditing(const TypeEditedEventDetails& editingEventDetails);

    // ------- State management
    // The editor's state is saved to disk and loaded back
    virtual void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) {}
    virtual void SaveState(nlohmann::json& json) const {}
};
} // namespace aln