#pragma once

#include "graph/conduit.hpp"
#include "graph/editor_graph.hpp"

#include "assets/animation_graph/nodes/state_editor_node.hpp"

#include <filesystem>

namespace aln
{

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
    Conduit* CreateConduit(const StateEditorNode* pStartState, const StateEditorNode* pEndState)
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

  public:
    const Vector<Conduit*>& GetConduits() const { return m_conduits; }

    virtual void RemoveGraphNode(const UUID& nodeID) override
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

    virtual void Clear() override
    {
        for (auto pConduit : m_conduits)
        {
            pConduit->Shutdown();
            aln::Delete(pConduit);
        }
        m_conduits.clear();

        EditorGraph::Clear();
    }

    virtual void FindAllNodesOfType(Vector<const EditorGraphNode*>& outResult, const StringID& typeID, NodeSearchScope searchScope) const override
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

    virtual void SaveState(JSON& json) const override
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

    virtual void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService)
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
};
} // namespace aln