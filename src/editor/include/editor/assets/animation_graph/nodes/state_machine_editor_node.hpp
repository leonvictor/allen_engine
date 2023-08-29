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
    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override
    {
    }

    virtual void SaveState(nlohmann::json& json) const override
    {
    }

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;

    virtual bool IsRenamable() const override { return true; }
};
} // namespace aln