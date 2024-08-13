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
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override {}
    void SaveState(JSON& json) const override {}

    void Initialize() override;
    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln