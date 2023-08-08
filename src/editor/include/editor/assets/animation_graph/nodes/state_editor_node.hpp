#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{

class TypeRegistryService;

class StateEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
  protected:
    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override
    {
    }

    virtual void SaveState(nlohmann::json& json) const override
    {
    }

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};
} // namespace aln