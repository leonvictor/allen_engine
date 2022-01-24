#pragma once

#include "../graph_node.hpp"
#include <vector>

namespace aln
{

class AnimationClipNode : public GraphNode
{
    struct Settings : public GraphNode::Settings
    {
        uint32_t m_dataSlotIdx; // From animationClipNode

        void InstanciateNode(const std::vector<GraphNode*>& nodePtrs, AnimationGraphDataSet const* pDataSet, InitOption options) override const
        {
            auto pNode = CreateNode<AnimationClipNode>(nodePtrs, options);
            SetOptionalNodePtrFromIndex(nodePtrs, m_playInReverseValueNodeIdx, pNode->m_pPlayInReverseValueNode);
            auto pSettings = pNode->GetSettings<AnimationClipNode>();
            pNode->m_pAnimation = pDataSet->GetAnimationClip(pSettings->m_dataSlotIdx);
        }
    };
};
} // namespace aln