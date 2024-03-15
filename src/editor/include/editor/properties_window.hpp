#pragma once

#include "editor_window.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <common/maths/angles.hpp>
#include <reflection/reflected_type.hpp>

namespace aln
{
/// @brief Display a reflected object's properties (i.e. Components, Systems, Graph Nodes)
class PropertiesWindow : public IEditorWindow
{
    reflect::IReflected* m_pInspectedObject = nullptr;

    UUID m_propertyEditingStartedEventID;
    UUID m_propertyEditingCompletedEventID;

    // TODO: Move to the quaternion widget directly
    EulerAnglesDegrees m_currentEulerRotation; // Inspector's rotation is stored separately to avoid going back and forth between quat and euler
    bool m_uniformScale = true;

    ReflectedTypeEditor m_reflectedTypeEditor;
    const TypeRegistryService* m_pTypeRegistryService = nullptr;

  public:
    void Initialize(EditorWindowContext* pEditorWindowContext) override;
    void Shutdown() override;
    virtual void Update(const UpdateContext& context);

    void BeginComponentEditing(const TypeEditedEventDetails& editingEventDetails);
    void EndComponentEditing(const TypeEditedEventDetails& editingEventDetails);

    // ------- State management
    // The editor's state is saved to disk and loaded back
    virtual void LoadState(JSON& json, const TypeRegistryService* pTypeRegistryService) {}
    virtual void SaveState(JSON& json) const {}
};
} // namespace aln