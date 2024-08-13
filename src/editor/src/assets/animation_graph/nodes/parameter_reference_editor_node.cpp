#include "assets/animation_graph/nodes/parameter_reference_editor_node.hpp"

#include "assets/animation_graph/nodes/control_parameter_editor_nodes.hpp"

#include <common/serialization/json.hpp>

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(NONE, ParameterReferenceEditorNode)
ALN_REFLECT_BASE(EditorAnimationGraphNode)
ALN_REGISTER_IMPL_END()

ParameterReferenceEditorNode::ParameterReferenceEditorNode(const IControlParameterEditorNode* pReferencedParameter) : m_pParameter(pReferencedParameter), m_parameterID(pReferencedParameter->GetName())
{
    assert(pReferencedParameter != nullptr);
}

void ParameterReferenceEditorNode::Initialize()
{
    assert(m_pParameter != nullptr);
    for (auto& pin : m_pParameter->GetOutputPins())
    {
        AddOutputPin(pin.GetValueType(), pin.GetName(), pin.AllowsMultipleLinks());
    }
}

NodeIndex ParameterReferenceEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    assert(m_pParameter != nullptr);
    return m_pParameter->Compile(context, graphDefinition);
};

void ParameterReferenceEditorNode::SaveState(JSON& json) const
{
    json["referenced_parameter"] = m_parameterID.GetHash();
}

void ParameterReferenceEditorNode::LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService)
{
    uint32_t parameterIDHash = json["referenced_parameter"];
    m_parameterID = StringID(parameterIDHash);
}

const std::string& ParameterReferenceEditorNode::GetName() const
{
    return m_pParameter->GetName();
}

} // namespace aln