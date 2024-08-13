#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

#include <anim/types.hpp>

namespace aln
{

class TypeRegistryService;
class TransitionEditorNode;

class StateMachineEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    NodeIndex CompileTransition(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition, const TransitionEditorNode* pTransitionNode, NodeIndex endStateNodeIdx) const;

  protected:
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override
    {
    }

    void SaveState(JSON& json) const override
    {
    }

  public:
    void Initialize() override;
    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;

    bool IsRenamable() const override { return true; }
};
} // namespace aln