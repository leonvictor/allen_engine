#pragma once

#include "../editor_graph_node.hpp"

namespace aln
{

class TypeRegistryService;

/// @brief Node responsible for blending between animations
class BlendEditorNode : public EditorGraphNode
{
    ALN_REGISTER_TYPE()

  public:
    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override {}

    virtual void SaveState(nlohmann::json& json) override {}

    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};
} // namespace aln