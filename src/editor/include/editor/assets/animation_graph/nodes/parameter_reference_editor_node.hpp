#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"
#include "control_parameter_editor_nodes.hpp"

namespace aln
{
/// @brief References an existing control parameter
class ParameterReferenceEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

    friend class EditorGraph;

  private:
    const IControlParameterEditorNode* m_pParameter = nullptr;
    StringID m_parameterID = StringID::InvalidID;

  public:
    ParameterReferenceEditorNode() = default;
    ParameterReferenceEditorNode(const IControlParameterEditorNode* pReferencedParameter) : m_pParameter(pReferencedParameter), m_parameterID(pReferencedParameter->GetName())
    {
        assert(pReferencedParameter != nullptr);
    }

    void Initialize() override
    {
        assert(m_pParameter != nullptr);
        for (auto& pin : m_pParameter->GetOutputPins())
        {
            AddOutputPin(pin.GetValueType(), pin.GetName(), pin.AllowsMultipleLinks());
        }
    }

    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override
    {
        assert(m_pParameter != nullptr);
        return m_pParameter->Compile(context, graphDefinition);
    };

    const std::string& GetName() const override { return m_pParameter->GetName(); }
    const IControlParameterEditorNode* GetReferencedParameter() const { return m_pParameter; }
    const StringID& GetReferencedParameterID() const { return m_parameterID; }

    void SaveState(nlohmann::json& json) const override
    {
        json["referenced_parameter"] = m_parameterID.GetHash();
    }

    void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override
    {
        uint32_t parameterIDHash = json["referenced_parameter"];
        m_parameterID = StringID(parameterIDHash);
    }
};
} // namespace aln