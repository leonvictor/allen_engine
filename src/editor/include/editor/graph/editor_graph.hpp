#pragma once

#include "asset_editor_workspace.hpp"
#include "graph/link.hpp"

#include <common/hash_vector.hpp>
#include <reflection/reflected_type.hpp>
#include <reflection/services/type_registry_service.hpp>
#include <common/containers/vector.hpp>

#include <imgui.h>
#include <imnodes.h>

#include <map>

namespace aln
{

class Pin;
class EditorGraphNode;

/// @brief Base class for in-editor graph types. This class holds the data, use GraphView for visualization
class EditorGraph : public reflect::IReflected
{
    ALN_REGISTER_TYPE();

    friend class GraphView;
    friend class EditorAnimationStateMachine;

  public:
    enum class NodeSearchScope : uint8_t
    {
        Local,
        Recursive,
    };

  private:
    EditorGraph* m_pParentGraph = nullptr;

    // TODO: Replace node vector + lookup map with IDVector
    Vector<EditorGraphNode*> m_graphNodes;
    IDVector<Link> m_links;

    std::map<UUID, const EditorGraphNode*> m_nodeLookupMap;
    std::map<UUID, const Pin*> m_pinLookupMap;

    // TODO: Dirty state might be shared behavior with other windows
    bool m_dirty = false;

    // UI Context
    ImNodesEditorContext* m_pImNodesEditorContext = nullptr;

    /// @brief Helper function used to add a node to the graph and register it in lookup maps
    void RegisterNode(EditorGraphNode* pNode);
    void RefreshParameterReferences();

  protected:
    const Vector<EditorGraphNode*>& GetNodes() const { return m_graphNodes; }

  public:
    ~EditorGraph();

    bool HasParentGraph() const { return m_pParentGraph != nullptr; }
    EditorGraph* GetParentGraph() const { return m_pParentGraph; }

    /// @brief Recursively find the root graph of the hierarchy
    const EditorGraph* GetRootGraph() const
    {
        if (HasParentGraph())
        {
            return m_pParentGraph->GetRootGraph();
        }
        else
        {
            return this;
        }
    }

    virtual void Initialize(EditorGraph* pParentGraph = nullptr);
    void Shutdown();
    bool IsInitialized() const { return m_pImNodesEditorContext != nullptr; }

    // TODO: Shared behavior ?
    bool IsDirty() const { return m_dirty; }
    void SetDirty() { m_dirty = true; }
    void SetClean() { m_dirty = false; }

    // ----------- Graph handling
    uint32_t GetNodeCount() const { return m_graphNodes.size(); }
    const EditorGraphNode* GetNode(const UUID& nodeID) const
    {
        assert(nodeID.IsValid());
        return m_nodeLookupMap.at(nodeID);
    }

    const EditorGraphNode* GetNodeByIndex(uint32_t nodeIndex) const { return m_graphNodes[nodeIndex]; }
    uint32_t GetNodeIndex(const UUID& nodeID) const;
    const EditorGraphNode* GetNodeLinkedToInputPin(const UUID& inputPinID) const;
    const EditorGraphNode* GetNodeLinkedToOutputPin(const UUID& outputPinID) const;

    virtual void FindAllNodesOfType(Vector<const EditorGraphNode*>& outResult, const StringID& typeID, NodeSearchScope searchScope = NodeSearchScope::Local) const;

    template <typename T>
    Vector<const T*> GetAllNodesOfType(NodeSearchScope searchScope = NodeSearchScope::Local) const
    {
        static_assert(std::is_base_of_v<EditorGraphNode, T>);
        Vector<const EditorGraphNode*> searchResult;
        FindAllNodesOfType(searchResult, T::GetStaticTypeInfo()->GetTypeID(), searchScope);

        Vector<const T*> matchingTypeNodes;
        matchingTypeNodes.reserve(searchResult.size());
        for (const auto pNode : searchResult)
        {
            const auto pTypedNode = static_cast<const T*>(pNode);
            matchingTypeNodes.push_back(pTypedNode);
        }

        return matchingTypeNodes;
    }

    template <typename T>
    Vector<T*> GetAllNodesOfType(NodeSearchScope searchScope = NodeSearchScope::Local)
    {
        static_assert(std::is_base_of_v<EditorGraphNode, T>);
        Vector<const EditorGraphNode*> searchResult;
        FindAllNodesOfType(searchResult, T::GetStaticTypeInfo()->GetTypeID(), searchScope);

        Vector<T*> matchingTypeNodes;
        matchingTypeNodes.reserve(searchResult.size());
        for (const auto pNode : searchResult)
        {
            // Static cast should be avoided, this is the lazy solution to avoid creating two versions of each getters
            const auto pTypedNode = const_cast<T*>(static_cast<const T*>(pNode));
            matchingTypeNodes.push_back(pTypedNode);
        }

        return matchingTypeNodes;
    }

    /// @brief Add a node to the graph
    EditorGraphNode* CreateGraphNode(const reflect::TypeInfo* pTypeInfo);

    template <typename T, typename... ConstructorParameters>
    T* CreateGraphNode(ConstructorParameters&&... parameters)
    {
        static_assert(std::is_base_of_v<EditorGraphNode, T>);
        auto pNode = aln::New<T>(std::forward<ConstructorParameters>(parameters)...);
        pNode->Initialize();
        RegisterNode(pNode);
        return pNode;
    }

    /// @brief Remove a node from the graph
    virtual void RemoveGraphNode(const UUID& nodeID);

    const Link* GetLink(const UUID& linkID) const
    {
        assert(linkID.IsValid());
        return &m_links.Get(linkID);
    }

    uint32_t GetLinkCount() const { return m_links.Size(); }
    /// @brief Create a link between two pins
    void AddLink(UUID startNodeID, UUID startPinID, UUID endNodeID, UUID endPinID);
    void RemoveLink(const UUID& linkID);

    void AddDynamicInputPin(EditorGraphNode* pNode);
    void RemoveDynamicInputPin(EditorGraphNode* pNode, const UUID& pinID);

    const Pin* GetPin(const UUID& pinID)
    {
        assert(pinID.IsValid());
        return m_pinLookupMap.at(pinID);
    }

    /// @note Only works for single-link pins
    const Link* GetLinkToPin(const UUID& pinID) const;

    // TODO: Make virtual and specialize in derived classes
    const reflect::TypeInfo* AvailableNodeTypesMenuItems(const TypeRegistryService* pTypeRegistryService)
    {
        auto& animGraphNodeTypes = pTypeRegistryService->GetTypesInScope("ANIM_GRAPH_EDITOR_NODES");
        for (auto& pAnimGraphNodeType : animGraphNodeTypes)
        {
            if (ImGui::MenuItem(pAnimGraphNodeType->m_name.c_str()))
            {
                return pAnimGraphNodeType;
            }
        }
        return nullptr;
    }

    /// @brief Clear the graph's content (nodes, links, etc.). Override in derived classes to clean up additional data
    virtual void Clear();

    // -------------- Saving/Loading
    virtual void SaveState(JSON& json) const;
    virtual void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService);
};

} // namespace aln