#pragma once

#include "graph/editor_graph.hpp"

#include "assets/animation_graph/nodes/state_editor_node.hpp"

#include <filesystem>

namespace aln
{


class EditorAnimationStateMachine : public EditorGraph
{
  private:
    bool m_isTransitionCreationInProgress = false;
    StateEditorNode* m_pTransitionStartState = nullptr;

  public:
    // -------------- Asset compilation
    //AnimationGraphDefinition* Compile(const std::filesystem::path& graphDefinitionPath, const std::filesystem::path& graphDatasetPath, const TypeRegistryService& typeRegistryService);
};
} // namespace aln