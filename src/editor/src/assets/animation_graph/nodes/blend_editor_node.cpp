#include "assets/animation_graph/nodes/blend_editor_node.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/blend_node.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, BlendEditorNode)
ALN_REGISTER_IMPL_END()

void BlendEditorNode::Initialize()
{
    m_name = "Blend";
    AddInputPin(NodeValueType::Float, "Blend Weight");
    
    AddInputPin(NodeValueType::Pose, "Input");
    m_blendParameterValues.push_back(0.0f);
    
    AddInputPin(NodeValueType::Pose, "Input");
    m_blendParameterValues.push_back(1.0f);

    AddOutputPin(NodeValueType::Pose, "Result");
}

NodeIndex BlendEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const
{
    BlendNode::Settings* pSettings = nullptr;
    bool compiled = context.GetSettings<BlendNode>(this, pGraphDefinition, pSettings);
    if (!compiled)
    {
        // Blend weight parameter
        const auto pBlendWeightValueNode = context.GetNodeLinkedToInputPin(GetInputPin(0).GetID());
        if (pBlendWeightValueNode == nullptr)
        {
            // TODO: Normalize error messages
            context.LogError("No input blend weight value node found.", this);
            return InvalidIndex;
        }
        pSettings->m_blendWeightValueNodeIdx = pBlendWeightValueNode->Compile(context, pGraphDefinition);
        
        // Sources 
        const auto inputPinsCount = GetInputPinsCount();
        pSettings->m_sourcePoseNodeIndices.reserve(inputPinsCount - 1);

        for (auto inputPinIdx = 1; inputPinIdx < inputPinsCount; ++inputPinIdx)
        {
            const auto pSourceNode = context.GetNodeLinkedToInputPin(GetInputPin(inputPinIdx).GetID());
            if (pSourceNode == nullptr)
            {
                context.LogError("No input node found at pin: Input " + std::to_string(inputPinIdx), this);
                return InvalidIndex;
            }
            pSettings->m_sourcePoseNodeIndices.push_back(pSourceNode->Compile(context, pGraphDefinition));
        }

        // Ranges
        // TODO: Ensure ranges are sorted by ascending values
        pSettings->m_blendRanges.reserve(inputPinsCount - 2);
        for (auto rangeIdx = 0; rangeIdx < inputPinsCount - 2; ++rangeIdx)
        {
            auto& range = pSettings->m_blendRanges.emplace_back();
            range.m_startNodeIndex = rangeIdx;
            range.m_endNodeIndex = rangeIdx + 1;
            range.m_startBlendWeightValue = m_blendParameterValues[rangeIdx];
            range.m_endBlendWeightValue = m_blendParameterValues[rangeIdx + 1];
        }
    }

    return pSettings->GetNodeIndex();
};

} // namespace aln
