#include "assets/animation_graph/nodes/control_parameter_editor_nodes.hpp"
#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/control_parameter_nodes.hpp>

namespace aln
{

ALN_REGISTER_ABSTRACT_IMPL_BEGIN(IControlParameterEditorNode)
ALN_REFLECT_BASE(EditorGraphNode)
ALN_REGISTER_IMPL_END()

// ------ Float control parameter node

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, FloatControlParameterEditorNode)
ALN_REFLECT_BASE(IControlParameterEditorNode)
ALN_REGISTER_IMPL_END()

void FloatControlParameterEditorNode::Initialize()
{
    m_name = "Float Parameter";
    AddOutputPin(NodeValueType::Float, "Value", true);
}

NodeIndex FloatControlParameterEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    FloatControlParameterNode::Settings* pSettings = nullptr;
    context.GetSettings<FloatControlParameterNode>(this, graphDefinition, pSettings);
    return pSettings->GetNodeIndex();
}

// ------ Bool control parameter node

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, BoolControlParameterEditorNode)
ALN_REFLECT_BASE(IControlParameterEditorNode)
ALN_REGISTER_IMPL_END()

void BoolControlParameterEditorNode::Initialize()
{
    m_name = "Bool Parameter";
    AddOutputPin(NodeValueType::Bool, "Value", true);
}

NodeIndex BoolControlParameterEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    BoolControlParameterNode::Settings* pSettings = nullptr;
    context.GetSettings<BoolControlParameterNode>(this, graphDefinition, pSettings);
    return pSettings->GetNodeIndex();
}

// ------ StringID control parameter node

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, IDControlParameterEditorNode)
ALN_REFLECT_BASE(IControlParameterEditorNode)
ALN_REGISTER_IMPL_END()

void IDControlParameterEditorNode::Initialize()
{
    m_name = "ID Parameter";
    AddOutputPin(NodeValueType::ID, "Value", true);
}

NodeIndex IDControlParameterEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    IDControlParameterNode::Settings* pSettings = nullptr;
    context.GetSettings<IDControlParameterNode>(this, graphDefinition, pSettings);
    return pSettings->GetNodeIndex();
}

// ------- TODO: ...

} // namespace aln