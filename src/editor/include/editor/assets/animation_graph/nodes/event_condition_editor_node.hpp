#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{
class EventConditionEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    // Simple string, converted to stringID at compile time. This way we can still serialize it
    std::string m_eventID;

  protected:
    void SaveState(JSON& json) const override;
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override;

  public:
    void Initialize() override;
    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln