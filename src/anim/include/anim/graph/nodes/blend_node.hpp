#pragma once

#include "../animation_graph_dataset.hpp"
#include "../pose_node.hpp"
#include "../runtime_graph_node.hpp"
#include "../tasks/blend_task.hpp"
#include "../value_node.hpp"

#include <vector>

namespace aln
{

/// @brief Node responsible for blending between animations.
/// @note For now, only two animations and interpolative blending are supported
class BlendNode : public PoseRuntimeNode
{
  private:
    ValueNode* m_pBlendWeightValueNode = nullptr;
    PoseRuntimeNode* m_pSourcePoseNode1 = nullptr;
    PoseRuntimeNode* m_pSourcePoseNode2 = nullptr;

    float m_blendWeight = 0.0;

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
        
        m_blendWeight = m_pBlendWeightValueNode->GetValue<float>(context);
        auto sourceNodeResult1 = m_pSourcePoseNode1->Update(context);
        auto sourceNodeResult2 = m_pSourcePoseNode2->Update(context);

        BitFlags<PoseBlend> blendOptions; // TODO
        result.m_taskIndex = context.m_pTaskSystem->RegisterTask<BlendTask>(GetNodeIndex(), sourceNodeResult1.m_taskIndex, sourceNodeResult2.m_taskIndex, m_blendWeight, blendOptions, nullptr);

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
        // TODO: Abstract method impl
        PoseRuntimeNode::InitializeInternal(context, initialTime);
     
        m_pBlendWeightValueNode->Initialize(context);
        m_pSourcePoseNode1->Initialize(context, initialTime);
        m_pSourcePoseNode2->Initialize(context, initialTime);
    }

    virtual void ShutdownInternal() override
    {
        m_pSourcePoseNode2->Shutdown();
        m_pSourcePoseNode1->Shutdown();
        m_pBlendWeightValueNode->Shutdown();

        PoseRuntimeNode::Shutdown();
    }
};
} // namespace aln 