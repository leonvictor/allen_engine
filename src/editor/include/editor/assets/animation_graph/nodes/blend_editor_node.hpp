#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{

class TypeRegistryService;

/// @brief Node responsible for blending between animations
class BlendEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    Vector<float> m_blendParameterValues;

  protected:
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override;
    void SaveState(JSON& json) const override;

    bool DrawPin(const Pin& pin, const GraphDrawingContext& ctx) override;

    void OnDynamicInputPinCreated(const UUID& pinID) override
    {
        m_blendParameterValues.push_back(1.0f);
    }

    void OnDynamicInputPinRemoved(const UUID& pinID) override
    {
        auto pinIdx = GetInputPinIndex(pinID);
        auto blendParameterIdx = pinIdx - 1;
        m_blendParameterValues.erase(m_blendParameterValues.begin() + blendParameterIdx);
    }

  public:
    bool SupportsDynamicInputPins() const override { return true; }
    NodeValueType DynamicInputPinValueType() const override { return NodeValueType::Pose; }
    std::string DynamicInputPinName() const override { return "Input"; }

    void Initialize() override;
    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln