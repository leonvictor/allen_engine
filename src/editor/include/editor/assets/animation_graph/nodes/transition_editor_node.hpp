#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{

class TypeRegistryService;

class TransitionEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

    friend class StateMachineEditorNode;

  private:
    float m_duration = 0.0f;

  protected:
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService);
    void SaveState(JSON& json) const override;

  public:
    void Initialize() override;
    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln