#pragma once

#include "../asset_editor_window.hpp"
#include "../reflected_types/reflected_type_editor.hpp"
#include "graph_drawing_context.hpp"
#include "link.hpp"

#include <anim/graph/runtime_graph_instance.hpp>

#include <map>
#include <vector>

namespace aln
{

class EditorGraphNode;

/// @brief A stateful representation of an animation graph, which contains all of its data.
/// During serialization its node are translated into the runtime format, and their data stored in a runtime graph definition
/// so that multiple instances of the same graph can share it.
/// @todo: Separate data/view
class AnimationGraphEditor : public IAssetEditorWindow
{
  private:
    AssetHandle<AnimationGraphDefinition> m_pGraphDefinition;

    std::filesystem::path m_compiledDefinitionPath;
    std::filesystem::path m_compiledDatasetPath;
    std::filesystem::path m_statePath;

    std::vector<EditorGraphNode*> m_graphNodes;
    std::map<UUID, const EditorGraphNode*> m_nodeLookupMap;
    std::map<UUID, const Pin*> m_pinLookupMap;
    std::vector<Link> m_links;

    UUID m_contextPopupElementID;

    bool m_waitingForAssetLoad = true;

    // TODO: Dirty state might be shared behavior with other windows
    bool m_dirty = false;
    bool m_isOpen = true;

    const TypeRegistryService* m_pTypeRegistryService = nullptr;

    GraphDrawingContext m_graphDrawingContext;

  public:
    // ----------- Window lifetime

    void Update(const UpdateContext& context) override;
    virtual void Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile) override;
    virtual void Shutdown() override;
    void Clear();

    bool IsDirty() const { return m_dirty; }
    void SetDirty() { m_dirty = true; }
    void SetClean() { m_dirty = false; }

    // ----------- Graph handling

    const EditorGraphNode* GetNode(const UUID& nodeID) const
    {
        assert(nodeID.IsValid());
        return m_nodeLookupMap.at(nodeID);
    }

    uint32_t GetNodeIndex(const UUID& nodeID) const;
    const EditorGraphNode* GetNodeLinkedToInputPin(const UUID& inputPinID) const;
    const EditorGraphNode* GetNodeLinkedToOutputPin(const UUID& outputPinID) const;

    template <typename T>
    std::vector<const T*> GetAllNodesOfType() const
    {
        static_assert(std::is_base_of_v<EditorGraphNode, T>);

        std::vector<const T*> matchingTypeNodes;
        for (const auto& [id, pNode] : m_nodeLookupMap)
        {
            const auto pTypedNode = dynamic_cast<const T*>(pNode);
            if (pTypedNode != nullptr)
            {
                matchingTypeNodes.push_back(pTypedNode);
            }
        }

        return matchingTypeNodes;
    }

    /// @brief Create a node to the graph
    void AddGraphNode(EditorGraphNode* pNode);

    /// @brief Remove a node from the graph
    void RemoveGraphNode(const UUID& nodeID);

    /// @brief Create a link between two pins
    void AddLink(UUID startNodeID, UUID startPinID, UUID endNodeID, UUID endPinID);
    void RemoveLink(const UUID& linkID);

    /// @note Only works for single-link pins
    const Link* GetLinkToPin(const UUID& pinID) const;

    // -------------- Saving/Loading

    AnimationGraphDefinition* Compile();

    virtual void SaveState(nlohmann::json& json) const override;
    virtual void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override;
};

ALN_ASSET_WINDOW_FACTORY(AnimationGraphDefinition, AnimationGraphEditor)

} // namespace aln