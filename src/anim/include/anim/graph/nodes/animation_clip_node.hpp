#pragma once

#include "../animation_graph_dataset.hpp"
#include "../pose_node.hpp"
#include "../tasks/sample_task.hpp"
#include "../value_node.hpp"
#include <vector>

namespace aln
{

/// @brief "Source" nodes, act as a slot in which data will get plugged in
class AnimationClipNode : public PoseNode
{
  public:
    struct Runtime : public PoseNode
    {
        Task* m_pTask;
        ValueNode* m_pPlayInReverseValueNode = nullptr; // TODO: Actually a boolNode
        AnimationClip* m_pAnimation = nullptr;

        Runtime()
        {
            // TODO: Attributes are not set when constructed. When do we create the task ?
            m_pTask = new SampleTask(0 /*todo*/, m_pAnimation, 0 /*todo*/);
        }
    };

    class Settings : public GraphNode::Settings
    {
      private:
        uint32_t m_dataSlotIdx;
        NodeIndex m_playInReverseValueNodeIdx = InvalidIndex;

      public:
        // From:
        // void AnimationClipNode::Settings::InstanciateNode(TVector<GraphNode*> const& nodePtrs, AnimationGraphDataset const* pDataset, InitOptions) const
        void InstanciateNode(const std::vector<GraphNode*>& nodePtrs, AnimationGraphDataSet const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<AnimationClipNode>(nodePtrs, options);
            SetOptionalNodePtrFromIndex(nodePtrs, m_playInReverseValueNodeIdx, pNode->m_pPlayInReverseValueNode);

            auto pSettings = pNode->GetSettings<AnimationClipNode>();
            pNode->m_pAnimation = pDataSet->GetAnimationClip(pSettings->m_dataSlotIdx);
        }
    };

    PoseNodeResult Update(GraphContext& context) override
    {
        // TODO: Abstract method impl
        PoseNodeResult result;
        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        // TODO: Abstract method impl
        PoseNodeResult result;
        return result;
    }

    virtual const SyncTrack& GetSyncTrack() const override{
        // TODO: Abstract method impl
    };

    virtual void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        // TODO: Abstract method impl
    }

  private:
};
} // namespace aln