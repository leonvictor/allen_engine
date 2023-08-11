#pragma once

#include "graph/editor_graph.hpp"
#include "graph/transition.hpp"

#include "assets/animation_graph/nodes/state_editor_node.hpp"

#include <filesystem>

namespace aln
{

class EditorAnimationStateMachine : public EditorGraph
{
    friend class GraphView;

  private:
    std::vector<EditorTransition*> m_transitions;

  private:
    EditorTransition* CreateTransition(const StateEditorNode* pStartState, const StateEditorNode* pEndState)
    {
        assert(pStartState != nullptr && pEndState != nullptr);

        auto pTransition = aln::New<EditorTransition>();
        pTransition->m_pStartState = pStartState;
        pTransition->m_pEndState = pEndState;
        pTransition->Initialize();

        m_transitions.push_back(pTransition);

        return pTransition;
    }

  public:
    void RemoveGraphNode(const UUID& nodeID) override
    {
        auto predicate = [&](const auto* pTransition)
        { return pTransition->GetStartState()->GetID() == nodeID || pTransition->GetEndState()->GetID() == nodeID; };

        // Erase-remove idiom but we delete each ptr
        // We might be able to use stable_partion as well
        auto first = std::find_if(m_transitions.begin(), m_transitions.end(), predicate);
        auto last = m_transitions.end();
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
        m_transitions.erase(first, m_transitions.end());

        EditorGraph::RemoveGraphNode(nodeID);
    }

    const std::vector<EditorTransition*>& GetTransitions() const { return m_transitions; }
};
} // namespace aln