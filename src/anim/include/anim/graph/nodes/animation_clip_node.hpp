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

            pNode->m_pAnimationClip = pDataSet->GetAnimationClip(m_dataSlotIdx);
        }
    };

    PoseNodeResult Update(GraphContext& context) override
    {
        assert(context.IsValid());

        // TODO: Mark the node as active

        if (m_pPlayInReverseValueNode != nullptr)
        {
            // TODO
        }

        const auto deltaPercentage = Percentage(context.m_deltaTime / m_duration);

        //auto pSettings = GetSettings<AnimationClipRuntimeNode>();

        m_previousTime = m_currentTime;
        m_currentTime += deltaPercentage;

        // TODO: Handle looping (or not)
        m_currentTime = std::fmod(m_currentTime, 1.0);

        PoseNodeResult result;
        result.m_taskIndex = context.m_pTaskSystem->RegisterTask<SampleTask>(GetNodeIndex(), m_pAnimationClip, m_currentTime);
        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        // TODO: Abstract method impl
        assert(false);
        PoseNodeResult result;
        return result;
    }

    virtual const SyncTrack& GetSyncTrack() const override
    {
        // TODO: Abstract method impl
        assert(false);
        return SyncTrack();
    };

    virtual void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        if (m_pPlayInReverseValueNode != nullptr)
        {
            m_pPlayInReverseValueNode->Initialize(context);
        }

        m_duration = m_pAnimationClip->GetDuration();

        // TODO: Initialize current time and previous time based on initialTime
    }

    virtual void ShutdownInternal() override
    {
        if (m_pPlayInReverseValueNode != nullptr)
        {
            m_pPlayInReverseValueNode->Shutdown();
        }
    }

    const AnimationClip* GetAnimationClip() const { return m_pAnimationClip; }
};
} // namespace aln