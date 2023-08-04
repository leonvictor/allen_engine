#pragma once

#include "../pose_node.hpp"
#include "state.hpp"

namespace aln
{

/// @brief Represents a transition from one state to another
class TransitionRuntimeNode : public PoseRuntimeNode
{
  public:
    struct Settings : public PoseRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;

      private:
        NodeIndex m_targetStateNodeIdx = InvalidIndex;
        float m_duration = 0.0f;

      public:
        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<TransitionRuntimeNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_targetStateNodeIdx, pNode->m_pTargetNode);
        }
    };

  private:
    float m_transitionProgress = 0.0f;
    PoseRuntimeNode* m_pSourceNode = nullptr;
    StateRuntimeNode* m_pTargetNode = nullptr;

  public:
    bool TransitionComplete() const { return m_transitionProgress >= 1.0f; }

    PoseNodeResult Update(GraphContext& context) override
    {
        PoseNodeResult result;
        // TODO
        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        // TODO
        PoseNodeResult result;
        return result;
    }

    void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        // TODO
    }

    void ShutdownInternal() override
    {
        // TODO
    }

    const SyncTrack& GetSyncTrack() const override
    {
        if (TransitionComplete())
        {
            return m_pTargetNode->GetSyncTrack();
        }
        else
        {
            return SyncTrack::Default; // TODO: ?
        }
    }
};
} // namespace aln