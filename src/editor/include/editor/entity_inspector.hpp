#pragma once

#include "editor_window.hpp"

namespace aln
{

class SpatialComponent;

class EntityInspector : public IEditorWindow
{
    Entity* m_pEntity;

    const TypeRegistryService* m_pTypeRegistryService = nullptr;

    void DrawSpatialComponentsHierarchy(SpatialComponent* pRootComponent);

  public:
    void Initialize(EditorWindowContext* pEditorWindowContext) override;
    void Shutdown() override;
    virtual void Update(const UpdateContext& context);

    // ------- State management
    // The editor's state is saved to disk and loaded back
    virtual void LoadState(JSON& json, const TypeRegistryService* pTypeRegistryService) {}
    virtual void SaveState(JSON& json) const {}
};
} // namespace aln