#pragma once

#include "../editor_graph_node.hpp"

namespace aln
{
class FloatControlParameterEditorNode : public EditorGraphNode
{
    ALN_REGISTER_TYPE();

  private:
    float m_value = 0.0f;

  public:
    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override
    {
        m_value = json["value"];
    }
    virtual void SaveState(nlohmann::json& json) override
    {
        json["value"] = m_value;
    }

    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};
} // namespace aln