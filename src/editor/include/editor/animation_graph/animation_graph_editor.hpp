#pragma once

#include <map>
#include <vector>

#include <imgui.h>
#include <imnodes.h>

#include <anim/graph/runtime_graph_instance.hpp>

#include "asset_editor_window.hpp"
#include "editor_graph_node.hpp"
#include "link.hpp"
#include "reflected_types/reflected_type_editor.hpp"

namespace aln
{

/// @brief A stateful representation of an animation graph, which contains all of its data.
/// During serialization its node are translated into the runtime format, and their data stored in a runtime graph definition
/// so that multiple instances of the same graph can share it.
/// @todo: Separate data/view
class AnimationGraphEditor : public IAssetEditorWindow
{
  private:
    AssetHandle<AnimationGraphDefinition> m_pGraphDefinition;

    // TODO: graphNodes and lookup map might be redundant
    std::vector<EditorGraphNode*> m_graphNodes;
    std::map<UUID, EditorGraphNode*> m_nodeLookupMap;
    std::map<UUID, const Pin*> m_pinLookupMap;
    std::vector<Link> m_links;

    UUID m_contextPopupElementID;

    bool m_waitingForAssetLoad = true;

    // TODO: Dirty state might be shared behavior with other windows
    bool m_dirty = false;
    bool m_shouldClose = false;

  public:
    void Update(const UpdateContext& context) override;

    ReflectedTypeEditor m_nodeInspector;

        // Loading a graph definitions ends up with a runtime graph def which we don't need.
        // Rather hand-load what we need from the archive
        // m_pGraphDefinition = AssetHandle<AnimationGraphDefinition>(id);
        // LoadAsset(m_pGraphDefinition);

        if (readAssetFile)
        {
            auto archive = BinaryFileArchive(id.GetAssetPath(), IBinaryArchive::IOMode::Read);
            AssetArchiveHeader header;

            archive >> header;

            // Deserialization
            size_t nodeCount;
            archive >> nodeCount;
        }

        // std::vector<std::type_index> nodeSettingsTypeIndices;
        // archive >> nodeSettingsTypeIndices;
    }

    virtual void Shutdown() override
    {
        IAssetEditorWindow::Shutdown();
        // if (m_pGraphDefinition.IsLoaded())
        // {
        //     UnloadAsset(m_pGraphDefinition);
        // }
    }

    bool IsDirty() { return m_dirty; }

    const EditorGraphNode* GetNode(const UUID& nodeID)
    {
        assert(nodeID.IsValid());
        return m_nodeLookupMap[nodeID];
    }

    const EditorGraphNode* GetNodeLinkedToInputPin(const UUID& inputPinID) const
    {
        for (auto& link : m_links)
        {
            if (link.m_inputPinID == inputPinID)
            {
                return link.m_pOutputNode;
            }
        }
        return nullptr;
    }

    const EditorGraphNode* GetNodeLinkedToOutputPin(const UUID& outputPinID) const
    {
        for (auto& link : m_links)
        {
            if (link.m_outputPinID == outputPinID)
            {
                return link.m_pInputNode;
            }
        }
        return nullptr;
    }

    template <typename T>
    std::vector<T*> GetAllNodesOfType()
    {
        static_assert(std::is_base_of_v<EditorGraphNode, T>);

        std::vector<T*> matchingTypeNodes;
        for (auto& [id, pNode] : m_nodeLookupMap)
        {
            auto pTypedNode = dynamic_cast<T*>(pNode);
            if (pTypedNode != nullptr)
            {
                matchingTypeNodes.push_back(pTypedNode);
            }
        }

        return matchingTypeNodes;
    }

    /// @brief Create a graph node of a selected type
    void AddGraphNode(EditorGraphNode* pNode)
    {
        assert(pNode != nullptr);
        m_dirty = true;

        // TODO: Add the node to the actual graph
        m_graphNodes.push_back(pNode);

        // Populate lookup maps
        m_nodeLookupMap[pNode->GetID()] = pNode;
        for (auto& pin : pNode->m_inputPins)
        {
            m_pinLookupMap[pin.GetID()] = &pin;
        }
        for (auto& pin : pNode->m_outputPins)
        {
            m_pinLookupMap[pin.GetID()] = &pin;
        }
    }

    /// @brief Remove a node from the graph
    void RemoveGraphNode(const UUID& nodeID)
    {
        assert(nodeID.IsValid());
        m_dirty = true;

        auto pNode = m_nodeLookupMap.at(nodeID);

        // Clean up lookup maps
        m_nodeLookupMap.erase(pNode->GetID());
        for (auto& pin : pNode->m_inputPins)
        {
            m_pinLookupMap.erase(pin.GetID());
        }
        for (auto& pin : pNode->m_outputPins)
        {
            m_pinLookupMap.erase(pin.GetID());
        }

        // Remove the node's attached links
        std::erase_if(m_links, [&](auto& link)
            { return link.m_pInputNode == pNode || link.m_pOutputNode == pNode; });

        // TODO: Actually remove the node from the graph
        std::erase(m_graphNodes, pNode);

        aln::Delete(pNode);
    }

    /// @brief Create a link between two pins
    void AddLink(UUID inputNodeID, UUID inputPinID, UUID outputNodeID, UUID outputPinID)
    {
        assert(inputNodeID.IsValid() && inputPinID.IsValid() && outputNodeID.IsValid() && outputPinID.IsValid());
        m_dirty = true;

        auto pInputPin = m_pinLookupMap[inputPinID];
        auto pOutputPin = m_pinLookupMap[outputPinID];

        // Ensure matching pin types
        if (pInputPin->GetValueType() != pOutputPin->GetValueType())
        {
            return;
        }

        // Ensure pins are input/output and in the right order
        if (!pInputPin->IsInput())
        {
            std::swap(pInputPin, pOutputPin);
            std::swap(inputNodeID, outputNodeID);
        }

        assert(pInputPin->IsInput() && pOutputPin->IsOutput());

        auto& link = m_links.emplace_back();
        link.m_pInputNode = m_nodeLookupMap[inputNodeID];
        link.m_inputPinID = pInputPin->GetID();
        link.m_pOutputNode = m_nodeLookupMap[outputNodeID];
        link.m_outputPinID = pOutputPin->GetID();
    }

    void RemoveLink(const UUID& linkID)
    {
        assert(linkID.IsValid());
        m_dirty = true;

        std::erase_if(m_links, [&](Link& link)
            { return link.m_id == linkID; });
    }

    /// @brief
    AnimationGraphDefinition* Compile();

    void LoadGraphDefinition(const AssetID& assetID)
    {
    }
};

ALN_ASSET_WINDOW_FACTORY(AnimationGraphDefinition, AnimationGraphEditor)

} // namespace aln