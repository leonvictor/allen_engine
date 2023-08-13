#pragma once

#include "asset_editor_workspace.hpp"
#include "assets/animation_graph/editor_animation_graph.hpp"
#include "graph/graph_view.hpp"

#include <reflection/services/type_registry_service.hpp>
#include <anim/graph/graph_definition.hpp>

namespace aln
{

class EditorAnimationGraphNode;

/// @brief Editing window for animation graphs. Holds a single animation graph, and displays several it through several views
class AnimationGraphWorkspace : public IAssetWorkspace
{
    struct GraphViewEventIDs
    {
        UUID m_nodeDoubleClickedEventID = UUID::InvalidID;
        UUID m_conduitDoubleClickedEventID = UUID::InvalidID;
        UUID m_canvasDoubleClickedEventID = UUID::InvalidID;
    };

  private:
    // TODO: Load this definition if necessary, and use it when compiling
    AssetHandle<AnimationGraphDefinition> m_pGraphDefinition;
    EditorAnimationGraph m_rootGraph;

    std::filesystem::path m_compiledDefinitionPath;
    std::filesystem::path m_compiledDatasetPath;
    std::filesystem::path m_statePath;

    // TODO: State might be shared behavior with other asset workspaces
    bool m_waitingForAssetLoad = true;
    bool m_dirty = false;
    bool m_isOpen = true;

    const TypeRegistryService* m_pTypeRegistryService = nullptr;

    GraphDrawingContext m_graphDrawingContext;
    GraphView m_primaryGraphView;
    GraphView m_secondaryGraphView;

    bool m_firstUpdate = true;
    float m_primaryGraphViewHeight = 0;
    float m_secondaryGraphViewHeight = 0;

    GraphViewEventIDs m_primaryGraphViewEventIDs;
    GraphViewEventIDs m_secondaryGraphViewEventIDs;

  public:
    // ----------- Window lifetime
    virtual void Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile) override;
    virtual void Shutdown() override;
    void Clear();
    void Update(const UpdateContext& context) override;

    bool IsDirty() const { return m_dirty; }
    void SetDirty() { m_dirty = true; }
    void SetClean() { m_dirty = false; }

    // -------------- Serialization
    virtual void SaveState(nlohmann::json& json) const override;
    virtual void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override;

    // -------------- Asset compilation
    void Compile();
};

ALN_ASSET_WORKSPACE_FACTORY(AnimationGraphDefinition, AnimationGraphWorkspace)

} // namespace aln