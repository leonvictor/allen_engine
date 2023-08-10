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
    void CreateTransition(const StateEditorNode* pStartState, const StateEditorNode* pEndState)
    {
        assert(pStartState != nullptr && pEndState != nullptr);

        auto pTransition = aln::New<EditorTransition>();
        pTransition->m_pStartState = pStartState;
        pTransition->m_pEndState = pEndState;

        m_transitions.push_back(pTransition);
    }

  public:
    ~EditorAnimationStateMachine()
    {
        for (auto& pTransition : m_transitions)
        {
            aln::Delete(pTransition);
        }
        EditorGraph::~EditorGraph();
    }

    // -------------- Asset compilation
    // AnimationGraphDefinition* Compile(const std::filesystem::path& graphDefinitionPath, const std::filesystem::path& graphDatasetPath, const TypeRegistryService& typeRegistryService);
};
} // namespace aln