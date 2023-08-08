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
    std::vector<float> m_blendParameterValues;

  protected:
    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override {
        m_blendParameterValues = json["blend_parameter_values"].get<std::vector<float>>();
        
        auto dynamicPinsCount = m_blendParameterValues.size() - 2;
        for (auto i = 0; i < dynamicPinsCount; ++i)
        {
            AddDynamicInputPin(DynamicInputPinValueType(), DynamicInputPinName());
        }

    }
    
    virtual void SaveState(nlohmann::json& json) const override {
        json["blend_parameter_values"] = m_blendParameterValues;
    }

    virtual bool DrawPin(const Pin& pin, const GraphDrawingContext& ctx) override
    {
        EditorAnimationGraphNode::DrawPin(pin, ctx);
        if (pin.IsInput() && pin.GetValueType() == NodeValueType::Pose)
        {
            auto pinIdx = GetInputPinIndex(pin.GetID());
            auto blendParameterIdx = pinIdx - 1;
            auto& blendParameterValue = m_blendParameterValues[blendParameterIdx];

            ImGui::PushID(&pin);
            ImGui::SetNextItemWidth(50);
            ImGui::InputFloat("", &blendParameterValue, 0.0f, 0.0f, "%.2f");
            ImGui::PopID();
        }
        return true;
    }

    virtual void OnDynamicInputPinCreated(const UUID& pinID) override {
        m_blendParameterValues.push_back(1.0f);
    }

    virtual void OnDynamicInputPinRemoved(const UUID& pinID) override {
        auto pinIdx = GetInputPinIndex(pinID);
        auto blendParameterIdx = pinIdx - 1;
        m_blendParameterValues.erase(m_blendParameterValues.begin() + blendParameterIdx);
    }

  public:
    virtual bool SupportsDynamicInputPins() const override { return true; }
    virtual NodeValueType DynamicInputPinValueType() const override { return NodeValueType::Pose; }
    virtual std::string DynamicInputPinName() const override { return "Input"; }

    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};
} // namespace aln