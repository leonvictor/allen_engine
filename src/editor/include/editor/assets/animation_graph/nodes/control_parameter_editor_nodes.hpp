#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{

/// @brief Base class for control parameters, allows easy filtering
/// @note Control parameters do not have default values to be as memory efficient as possible
/// @todo Control Parameters names must be unique
class IControlParameterEditorNode : public EditorAnimationGraphNode
{
    virtual bool IsRenamable() const final override { return true; }
};

class FloatControlParameterEditorNode : public IControlParameterEditorNode
{
    ALN_REGISTER_TYPE();

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};

class BoolControlParameterEditorNode : public IControlParameterEditorNode
{
    ALN_REGISTER_TYPE();

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};

class IDControlParameterEditorNode : public IControlParameterEditorNode
{
    ALN_REGISTER_TYPE();

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};

// TODO: ...

} // namespace aln