#include "assets/animation_graph/editor_animation_state_machine.hpp"

#include "assets/animation_graph/nodes/state_editor_node.hpp"
#include "graph/conduit.hpp"

#include <common/serialization/json.hpp>

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(GRAPH_EDITORS, EditorAnimationStateMachine)
ALN_REFLECT_BASE(EditorGraph)
ALN_REGISTER_IMPL_END()

Conduit* EditorAnimationStateMachine::CreateConduit(const StateEditorNode* pStartState, const StateEditorNode* pEndState)
{
    assert(pStartState != nullptr && pEndState != nullptr);

    auto pConduit = aln::New<Conduit>();
    pConduit->m_pStartState = pStartState;
    pConduit->m_pEndState = pEndState;
    pConduit->Initialize();
    pConduit->m_pChildGraph->m_pParentGraph = this;

    m_conduits.push_back(pConduit);

    return pConduit;
}

void EditorAnimationStateMachine::RemoveGraphNode(const UUID& nodeID)
{
    auto predicate = [&](const auto* pConduit)
    { return pConduit->GetStartState()->GetID() == nodeID || pConduit->GetEndState()->GetID() == nodeID; };

    // Erase-remove idiom but we delete each ptr
    // We might be able to use stable_partion as well
    auto first = std::find_if(m_conduits.begin(), m_conduits.end(), predicate);
    auto last = m_conduits.end();
    if (first != last)
    {
        for (auto it = first; it != last; ++it)
        {
            if (!predicate(*it))
            {
                *first = std::move(*it);
                first++;
            }
            else
            {
                (*it)->Shutdown();
                aln::Delete(*it);
            }
        }
    }
    m_conduits.erase(first, m_conduits.end());

    EditorGraph::RemoveGraphNode(nodeID);
}

void EditorAnimationStateMachine::Clear()
{
    for (auto pConduit : m_conduits)
    {
        pConduit->Shutdown();
        aln::Delete(pConduit);
    }
    m_conduits.clear();

    EditorGraph::Clear();
}

void EditorAnimationStateMachine::FindAllNodesOfType(Vector<const EditorGraphNode*>& outResult, const StringID& typeID, NodeSearchScope searchScope) const
{
    if (searchScope == NodeSearchScope::Recursive)
    {
        for (auto& conduit : m_conduits)
        {
            if (conduit->HasChildGraph())
            {
                conduit->GetChildGraph()->FindAllNodesOfType(outResult, typeID, searchScope);
            }
        }
    }

    EditorGraph::FindAllNodesOfType(outResult, typeID, searchScope);
}

void EditorAnimationStateMachine::SaveState(JSON& json) const
{
    EditorGraph::SaveState(json);
    auto& conduitsArrayJson = json["conduits"];
    for (auto pConduit : m_conduits)
    {
        auto& conduitJson = conduitsArrayJson.emplace_back();
        conduitJson["start_state"] = GetNodeIndex(pConduit->m_pStartState->GetID());
        conduitJson["end_state"] = GetNodeIndex(pConduit->m_pEndState->GetID());

        if (pConduit->HasChildGraph())
        {
            pConduit->m_pChildGraph->SaveState(conduitJson["child_graph"]);
        }
    }
}

void EditorAnimationStateMachine::LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService)
{
    EditorGraph::LoadState(json, pTypeRegistryService);

    for (const auto& conduitJson : json["conduits"])
    {
        const auto pStartState = static_cast<const StateEditorNode*>(GetNodeByIndex(conduitJson["start_state"]));
        const auto pEndState = static_cast<const StateEditorNode*>(GetNodeByIndex(conduitJson["end_state"]));

        auto pConduit = CreateConduit(pStartState, pEndState);

        if (conduitJson.contains("child_graph"))
        {
            pConduit->m_pChildGraph->LoadState(conduitJson["child_graph"], pTypeRegistryService);
        }
    }
}
} // namespace aln