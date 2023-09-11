#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{
class TypeRegistryService;

class BoolNotEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};

class BoolAndEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};

class BoolOrEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln