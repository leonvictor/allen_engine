#pragma once

#include "../animation_graph_dataset.hpp"
#include "../pose_node.hpp"
#include "../runtime_graph_node.hpp"
#include "../tasks/sample_task.hpp"
#include "../value_node.hpp"

#include <vector>

namespace aln
{

/// @brief "Source" nodes, act as a slot in which data will get plugged in
class AnimationClipRuntimeNode : public PoseRuntimeNode
{
  private:
    ValueNode* m_pPlayInReverseValueNode = nullptr; // TODO: Actually a boolNode
    const AnimationClip* m_pAnimationClip = nullptr;

  public:
    class Settings : public RuntimeGraphNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationClipEditorNode;

      private:
        uint32_t m_dataSlotIdx;
        NodeIndex m_playInReverseValueNodeIdx = InvalidIndex;

      public:
        void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, const AnimationGraphDataset* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<AnimationClipRuntimeNode>(nodePtrs, options);
            SetOptionalNodePtrFromIndex(nodePtrs, m_playInReverseValueNodeIdx, pNode->m_pPlayInReverseValueNode);

            auto pSettings = pNode->GetSettings<AnimationClipRuntimeNode>();
            pNode->m_pAnimationClip = pDataSet->GetAnimationClip(pSettings->m_dataSlotIdx);
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

    virtual const SyncTrack& GetSyncTrack() const override
    {
        // TODO: Abstract method impl
        return SyncTrack();
    };

    virtual void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        // TODO: Abstract method impl
    }

    const AnimationClip* GetAnimationClip() const { return m_pAnimationClip; }

  private:
};
} // namespace aln