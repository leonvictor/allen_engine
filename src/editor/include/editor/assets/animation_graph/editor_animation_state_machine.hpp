#pragma once

#include "graph/editor_graph.hpp"

#include <filesystem>

namespace aln
{
class Conduit;
class StateEditorNode;

class EditorAnimationStateMachine : public EditorGraph
{
    ALN_REGISTER_TYPE()

    friend class GraphView;

  private:
    Vector<Conduit*> m_conduits;

    // TODO:
    // Entry State Overrides
    // Global transitions

  private:
    Conduit* CreateConduit(const StateEditorNode* pStartState, const StateEditorNode* pEndState);

  public:
    const Vector<Conduit*>& GetConduits() const { return m_conduits; }

    void RemoveGraphNode(const UUID& nodeID) override;

    void Clear() override;

    void FindAllNodesOfType(Vector<const EditorGraphNode*>& outResult, const StringID& typeID, NodeSearchScope searchScope) const override;

    void SaveState(JSON& json) const override;
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService);
};
} // namespace aln