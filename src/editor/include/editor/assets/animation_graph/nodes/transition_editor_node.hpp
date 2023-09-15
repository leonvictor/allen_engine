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
    virtual void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override
    {
        m_duration = json["duration"];
    }

    virtual void SaveState(JSON& json) const override
    {
        json["duration"] = m_duration;
    }

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln