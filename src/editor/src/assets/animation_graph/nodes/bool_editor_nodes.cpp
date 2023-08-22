#include "assets/animation_graph/nodes/bool_editor_nodes.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/bool_nodes.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, BoolNotEditorNode)
ALN_REGISTER_IMPL_END()

void BoolNotEditorNode::Initialize()
{
    m_name = "Not";
    AddInputPin(NodeValueType::Bool, "Value");
    AddOutputPin(NodeValueType::Bool, "Result", true);
}

NodeIndex BoolNotEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    BoolNotRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<BoolNotRuntimeNode>(this, graphDefinition, pSettings);
    if (!compiled)
    {
        const auto pInputNode = context.GetNodeLinkedToInputPin(GetInputPin(0).GetID());
        if (pInputNode == nullptr)
        {
            context.LogError("No node linked to input pin " + GetInputPin(0).GetName(), this);
            return InvalidIndex;
        }
        pSettings->m_inputValueNodeIdx = pInputNode->Compile(context, graphDefinition);
    }

    return pSettings->GetNodeIndex();
};

} // namespace aln
