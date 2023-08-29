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
/// @todo Range-based blend : 1D Blend between multiple animations based on associated weights
/// ex: 0 -> Idle; 0.5 -> Walk; 1 -> Run
class BlendNode : public PoseRuntimeNode
{
    /// @brief Associates source nodes with a blend weight range
    struct BlendRange
    {
        // Indices of source nodes *in the blend node's arrays*
        uint32_t m_startNodeIndex = InvalidIndex;
        uint32_t m_endNodeIndex = InvalidIndex;
        float m_startBlendWeightValue = 0.0f;
        float m_endBlendWeightValue = 1.0f;

        bool Contains(float blendWeightValue) const { return m_startBlendWeightValue <= blendWeightValue && m_endBlendWeightValue >= blendWeightValue; }
    };

  private:
    ValueNode* m_pBlendWeightValueNode = nullptr;
    std::vector<PoseRuntimeNode*> m_sourceNodes;

    float m_blendWeight = 0.0;
    SyncTrack m_blendedSyncTrack;

    const BlendRange& SelectBlendRange(float blendWeightValue)
    {
        const auto pSettings = GetSettings<BlendNode>();
        for (const auto& range : pSettings->GetBlendRanges())
        {
            if (range.Contains(blendWeightValue))
            {
                return range;
            }
        }
        assert(false);
    }

  public:
    class Settings : public PoseRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class BlendEditorNode;

      private:
        NodeIndex m_blendWeightValueNodeIdx = InvalidIndex;
        std::vector<NodeIndex> m_sourcePoseNodeIndices;
        std::vector<BlendRange> m_blendRanges;

      public:
        void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, const AnimationGraphDataset* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<BlendNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_blendWeightValueNodeIdx, pNode->m_pBlendWeightValueNode);
            
            auto sourceNodesCount = m_sourcePoseNodeIndices.size(); 
            pNode->m_sourceNodes.resize(sourceNodesCount, nullptr);
            for (auto sourceNodeIdx = 0; sourceNodeIdx < sourceNodesCount; ++sourceNodeIdx)
            {
                SetNodePtrFromIndex(nodePtrs, m_sourcePoseNodeIndices[sourceNodeIdx], pNode->m_sourceNodes[sourceNodeIdx]);
            }
        }
    
        const std::vector<BlendRange>& GetBlendRanges() const { return m_blendRanges; }
    };

    PoseNodeResult Update(GraphContext& context) override
    {
        assert(context.IsValid());

        PoseNodeResult result;

        // TODO: Avoid updating source nodes if it's not necessary (i.e. we're at 0 or 1)
        m_blendWeight = m_pBlendWeightValueNode->GetValue<float>(context);
        const auto& blendRange = SelectBlendRange(m_blendWeight);
        auto scaledBlendWeight = (m_blendWeight - blendRange.m_startBlendWeightValue) / (blendRange.m_endBlendWeightValue - blendRange.m_startBlendWeightValue);  

        auto pSourceNode = m_sourceNodes[blendRange.m_startNodeIndex];
        auto pTargetNode = m_sourceNodes[blendRange.m_endNodeIndex];

        const auto& sourceSyncTrack = pSourceNode->GetSyncTrack();
        const auto& targetSyncTrack = pTargetNode->GetSyncTrack();
        m_blendedSyncTrack = SyncTrack::Blend(sourceSyncTrack, targetSyncTrack, scaledBlendWeight);

        const auto deltaPercentage = context.m_deltaTime / m_duration;
        SyncTrackTimeRange timeRange;
        timeRange.m_beginTime = m_blendedSyncTrack.GetTime(m_currentTime);

        float integralPart;
        timeRange.m_endTime = m_blendedSyncTrack.GetTime(std::modff(m_currentTime + deltaPercentage, &integralPart));

        const auto sourceNodeResult = pSourceNode->Update(context, timeRange);
        const auto targetNodeResult = pTargetNode->Update(context, timeRange);

        BitFlags<PoseBlend> blendOptions; // TODO
        result.m_taskIndex = context.m_pTaskSystem->RegisterTask<BlendTask>(GetNodeIndex(), sourceNodeResult.m_taskIndex, targetNodeResult.m_taskIndex, scaledBlendWeight, blendOptions, nullptr);
        // TODO: we could skip an op by not interpolating scale (which is not used by the root motion track)
        result.m_rootMotionDelta = Transform::Interpolate(sourceNodeResult.m_rootMotionDelta, targetNodeResult.m_rootMotionDelta, scaledBlendWeight);

        // Update time state
        m_duration = SyncTrack::CalculateSynchronizedTrackDuration(pSourceNode->GetDuration(), pTargetNode->GetDuration(), sourceSyncTrack, targetSyncTrack, m_blendedSyncTrack, scaledBlendWeight);
        m_previousTime = m_blendedSyncTrack.GetPercentageThrough(timeRange.m_beginTime);
        m_currentTime = m_blendedSyncTrack.GetPercentageThrough(timeRange.m_endTime);

        // TODO: Unsynced
        // auto sourceNodeResult1 = m_pSourcePoseNode1->Update(context);
        // auto sourceNodeResult2 = m_pSourcePoseNode2->Update(context);

        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        // TODO: Abstract method impl
        assert(false);
        PoseNodeResult result;
        return result;
    }

    virtual const SyncTrack& GetSyncTrack() const override { return m_blendedSyncTrack; };

    virtual void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        PoseRuntimeNode::InitializeInternal(context, initialTime);

        m_pBlendWeightValueNode->Initialize(context);
        
        auto sourceNodesCount = m_sourceNodes.size();
        for (auto pSourceNode : m_sourceNodes)
        {
            pSourceNode->Initialize(context, initialTime);
        }

        m_blendWeight = m_pBlendWeightValueNode->GetValue<float>(context);
    }

    virtual void ShutdownInternal() override
    {
        for (auto pSourceNode : m_sourceNodes)
        {
            pSourceNode->Shutdown();
        }
        m_pBlendWeightValueNode->Shutdown();

        PoseRuntimeNode::ShutdownInternal();
    }
};
} // namespace aln