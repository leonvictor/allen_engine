#include "animation_graph/nodes/blend_editor_node.hpp"

#include "animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/blend_node.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, BlendEditorNode)
ALN_REGISTER_IMPL_END()

void BlendEditorNode::Initialize()
{
    m_name = "Blend";
    AddInputPin(NodeValueType::Pose, "Source Pose");
    AddInputPin(NodeValueType::Pose, "Target Pose");
    AddInputPin(NodeValueType::Float, "Blend Weight");

    AddOutputPin(NodeValueType::Pose, "Result");
}

NodeIndex BlendEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
{
    BlendNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<BlendNode>(this, pGraphDefinition, pSettings);
    if (!compiled)
    {
        const auto pSourceNode1 = context.GetNodeLinkedToInputPin(GetInputPin(0).GetID());
        const auto pSourceNode2 = context.GetNodeLinkedToInputPin(GetInputPin(1).GetID());
        const auto pBlendWeightValueNode = context.GetNodeLinkedToInputPin(GetInputPin(2).GetID());
        
        pSettings->m_blendWeightValueNodeIdx = pBlendWeightValueNode->Compile(context, pGraphDefinition);
        pSettings->m_sourcePoseNode1Idx = pSourceNode1->Compile(context, pGraphDefinition);
        pSettings->m_sourcePoseNode2Idx = pSourceNode2->Compile(context, pGraphDefinition);
    }

    return pSettings->GetNodeIndex();
};

} // namespace aln
