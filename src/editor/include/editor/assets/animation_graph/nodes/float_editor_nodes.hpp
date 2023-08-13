#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{

class TypeRegistryService;

class FloatClampEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    float m_min = 0.0f;
    float m_max = 1.0f;

  public:
    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override {}
    virtual void SaveState(nlohmann::json& json) const override {}

    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln