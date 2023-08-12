#include "assets/animation_graph/nodes/float_editor_nodes.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/float_nodes.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, FloatClampEditorNode)
ALN_REFLECT_MEMBER(m_min)
ALN_REFLECT_MEMBER(m_max)
ALN_REGISTER_IMPL_END()

void FloatClampEditorNode::Initialize()
{
    m_name = "Clamp";
    AddInputPin(NodeValueType::Float, "Value");
    AddOutputPin(NodeValueType::Float, "Result", true);
}

NodeIndex FloatClampEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
{
    FloatClampRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<FloatClampRuntimeNode>(this, pGraphDefinition, pSettings);
    if (!compiled)
    {
        const auto pInputNode = context.GetNodeLinkedToInputPin(GetInputPin(0).GetID());
        if (pInputNode == nullptr)
        {
            context.LogError("No node linked to input pin " + GetInputPin(0).GetName(), this);
            return InvalidIndex;
        }
        pSettings->m_inputValueNodeIdx = pInputNode->Compile(context, pGraphDefinition);
        pSettings->m_min = m_min;
        pSettings->m_max = m_max;
    }

    return pSettings->GetNodeIndex();
};

} // namespace aln
