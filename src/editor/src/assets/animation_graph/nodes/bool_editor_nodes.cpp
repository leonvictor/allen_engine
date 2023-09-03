#include "assets/animation_graph/nodes/bool_editor_nodes.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/bool_nodes.hpp>

namespace aln
{
// ---- OR

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

// ---- AND

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, BoolAndEditorNode)
ALN_REGISTER_IMPL_END()

void BoolAndEditorNode::Initialize()
{
    m_name = "And";
    AddInputPin(NodeValueType::Bool, "Value");
    AddInputPin(NodeValueType::Bool, "Value");
    AddOutputPin(NodeValueType::Bool, "Result", true);
}

NodeIndex BoolAndEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    BoolAndRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<BoolAndRuntimeNode>(this, graphDefinition, pSettings);
    if (!compiled)
    {
        const auto pInputNode1= context.GetNodeLinkedToInputPin(GetInputPin(0).GetID());
        if (pInputNode1 == nullptr)
        {
            context.LogError("No node linked to input pin " + GetInputPin(0).GetName(), this);
            return InvalidIndex;
        }

        const auto pInputNode2 = context.GetNodeLinkedToInputPin(GetInputPin(1).GetID());
        if (pInputNode2 == nullptr)
        {
            context.LogError("No node linked to input pin " + GetInputPin(1).GetName(), this);
            return InvalidIndex;
        }

        pSettings->m_inputValueNode1Idx = pInputNode1->Compile(context, graphDefinition);
        pSettings->m_inputValueNode2Idx = pInputNode2->Compile(context, graphDefinition);

    }

    return pSettings->GetNodeIndex();
};

// ---- Or

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, BoolOrEditorNode)
ALN_REGISTER_IMPL_END()

void BoolOrEditorNode::Initialize()
{
    m_name = "Or";
    AddInputPin(NodeValueType::Bool, "Value");
    AddInputPin(NodeValueType::Bool, "Value");
    AddOutputPin(NodeValueType::Bool, "Result", true);
}

NodeIndex BoolOrEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    BoolOrRuntimeNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<BoolOrRuntimeNode>(this, graphDefinition, pSettings);
    if (!compiled)
    {
        const auto pInputNode1 = context.GetNodeLinkedToInputPin(GetInputPin(0).GetID());
        if (pInputNode1 == nullptr)
        {
            context.LogError("No node linked to input pin " + GetInputPin(0).GetName(), this);
            return InvalidIndex;
        }

        const auto pInputNode2 = context.GetNodeLinkedToInputPin(GetInputPin(1).GetID());
        if (pInputNode2 == nullptr)
        {
            context.LogError("No node linked to input pin " + GetInputPin(1).GetName(), this);
            return InvalidIndex;
        }

        pSettings->m_inputValueNode1Idx = pInputNode1->Compile(context, graphDefinition);
        pSettings->m_inputValueNode2Idx = pInputNode2->Compile(context, graphDefinition);
    }

    return pSettings->GetNodeIndex();
};

} // namespace aln
