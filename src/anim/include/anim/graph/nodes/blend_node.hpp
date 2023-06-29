#pragma once

#include "../pose_node.hpp"
#include "../runtime_graph_node.hpp"
#include "../tasks/blend_task.hpp"
#include "../value_node.hpp"

#include <vector>

namespace aln
{

class AnimationGraphDataset;

/// @brief Node responsible for blending between animations.
/// @note For now, only two animations and interpolative blending are supported
class BlendNode : public PoseRuntimeNode
{
  private:
    ValueNode* m_pBlendWeightValueNode = nullptr;
    PoseRuntimeNode* m_pSourcePoseNode1 = nullptr;
    PoseRuntimeNode* m_pSourcePoseNode2 = nullptr;

    float m_blendWeight = 0.0;
    SyncTrack m_blendedSyncTrack;

  public:
    class Settings : public PoseRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class BlendEditorNode;

      private:
        NodeIndex m_blendWeightValueNodeIdx = InvalidIndex;
        NodeIndex m_sourcePoseNode1Idx = InvalidIndex;
        NodeIndex m_sourcePoseNode2Idx = InvalidIndex;

      public:
        void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, const AnimationGraphDataset* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<BlendNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_blendWeightValueNodeIdx, pNode->m_pBlendWeightValueNode);
            SetNodePtrFromIndex(nodePtrs, m_sourcePoseNode1Idx, pNode->m_pSourcePoseNode1);
            SetNodePtrFromIndex(nodePtrs, m_sourcePoseNode2Idx, pNode->m_pSourcePoseNode2);            
        }
    };

    PoseNodeResult Update(GraphContext& context) override
    {
        assert(context.IsValid());

        // TODO: Mark the node as active
        
        PoseNodeResult result;
        
        // TODO: Avoid updating source nodes if it's not necessary (i.e. we're at 0 or 1)
        m_blendWeight = m_pBlendWeightValueNode->GetValue<float>(context);
        
        const auto deltaPercentage = context.m_deltaTime / m_duration; 
        SyncTrackTimeRange timeRange;
        timeRange.m_beginTime = m_blendedSyncTrack.GetTime(m_currentTime);
        
        float integralPart;
        timeRange.m_endTime = m_blendedSyncTrack.GetTime(std::modff(m_currentTime + deltaPercentage, &integralPart));

        const auto sourceNodeResult = m_pSourcePoseNode1->Update(context, timeRange);
        const auto targetNodeResult = m_pSourcePoseNode2->Update(context, timeRange);

        BitFlags<PoseBlend> blendOptions; // TODO
        result.m_taskIndex = context.m_pTaskSystem->RegisterTask<BlendTask>(GetNodeIndex(), sourceNodeResult.m_taskIndex, targetNodeResult.m_taskIndex, m_blendWeight, blendOptions, nullptr);
        // TODO: we could skip an op by not interpolating scale (which is not used by the root motion track)
        result.m_rootMotionDelta = Transform::Interpolate(sourceNodeResult.m_rootMotionDelta, targetNodeResult.m_rootMotionDelta, m_blendWeight);

        // TODO: Before ?
        const auto& sourceSyncTrack = m_pSourcePoseNode1->GetSyncTrack();
        const auto& targetSyncTrack = m_pSourcePoseNode2->GetSyncTrack();
        m_blendedSyncTrack = SyncTrack::Blend(sourceSyncTrack, targetSyncTrack, m_blendWeight);
        
        m_duration = SyncTrack::CalculateSynchronizedTrackDuration(m_pSourcePoseNode1->GetDuration(), m_pSourcePoseNode2->GetDuration(), sourceSyncTrack, targetSyncTrack, m_blendedSyncTrack, m_blendWeight);
        m_previousTime = m_blendedSyncTrack.GetPercentageThrough(timeRange.m_beginTime);
        m_currentTime = m_blendedSyncTrack.GetPercentageThrough(timeRange.m_endTime);

        // TODO: Unsynced 
        //auto sourceNodeResult1 = m_pSourcePoseNode1->Update(context);
        //auto sourceNodeResult2 = m_pSourcePoseNode2->Update(context);

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
        return m_blendedSyncTrack;
    };

    virtual void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        PoseRuntimeNode::InitializeInternal(context, initialTime);
     
        m_pBlendWeightValueNode->Initialize(context);
        m_pSourcePoseNode1->Initialize(context, initialTime);
        m_pSourcePoseNode2->Initialize(context, initialTime);

        m_blendWeight = m_pBlendWeightValueNode->GetValue<float>(context);

        const auto& sourceSyncTrack = m_pSourcePoseNode1->GetSyncTrack();
        const auto& targetSyncTrack = m_pSourcePoseNode2->GetSyncTrack();
        m_blendedSyncTrack = SyncTrack::Blend(sourceSyncTrack, targetSyncTrack, m_blendWeight);
        m_duration = SyncTrack::CalculateSynchronizedTrackDuration(m_pSourcePoseNode1->GetDuration(), m_pSourcePoseNode2->GetDuration(), sourceSyncTrack, targetSyncTrack, m_blendedSyncTrack, m_blendWeight);
    }

    virtual void ShutdownInternal() override
    {
        m_pSourcePoseNode2->Shutdown();
        m_pSourcePoseNode1->Shutdown();
        m_pBlendWeightValueNode->Shutdown();

        PoseRuntimeNode::ShutdownInternal();
    }
};
} // namespace aln 