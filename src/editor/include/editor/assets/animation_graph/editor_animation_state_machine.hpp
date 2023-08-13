#pragma once

#include "graph/conduit.hpp"
#include "graph/editor_graph.hpp"

#include "assets/animation_graph/nodes/state_editor_node.hpp"

#include <filesystem>

namespace aln
{

class EditorAnimationStateMachine : public EditorGraph
{
    friend class GraphView;

  private:
    std::vector<Conduit*> m_conduits;

  private:
    Conduit* CreateConduit(const StateEditorNode* pStartState, const StateEditorNode* pEndState)
    {
        assert(pStartState != nullptr && pEndState != nullptr);

        auto pConduit = aln::New<Conduit>();
        pConduit->m_pStartState = pStartState;
        pConduit->m_pEndState = pEndState;
        pConduit->Initialize(this);

        m_conduits.push_back(pConduit);

        return pConduit;
    }

  public:
    const std::vector<Conduit*>& GetConduits() const { return m_conduits; }

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

};
} // namespace aln