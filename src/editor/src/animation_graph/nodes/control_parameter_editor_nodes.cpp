#include "animation_graph/nodes/control_parameter_editor_nodes.hpp"

#include <anim/graph/nodes/control_parameter_nodes.hpp>

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, FloatControlParameterEditorNode)
ALN_REFLECT_MEMBER(m_value, Value);
ALN_REGISTER_IMPL_END()

void FloatControlParameterEditorNode::Initialize()
{
    m_name = "Float Parameter";
    AddOutputPin(PinValueType::Float, "Value", true);
}

NodeIndex FloatControlParameterEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
{
    FloatControlParameterNode::Settings* pSettings = nullptr;
    context.GetSettings<FloatControlParameterNode>(this, pGraphDefinition, pSettings);
    return pSettings->GetNodeIndex();
}
}