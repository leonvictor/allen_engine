#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{

class TypeRegistryService;

class StateEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    void PushNodeStyle(const GraphDrawingContext& ctx) const override
    {
    }

    void PopNodeStyle(const GraphDrawingContext& ctx) const override
    {
    }

  protected:
    virtual void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override
    {
    }

    virtual void SaveState(JSON& json) const override
    {
    }

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
    virtual bool IsRenamable() const override { return true; }
};
} // namespace aln