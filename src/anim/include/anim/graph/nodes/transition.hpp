#pragma once

#include "../pose_node.hpp"
#include "../value_node.hpp"
#include "state.hpp"

namespace aln
{

/// @brief Represents a transition from one state to another
/// @note https://www.youtube.com/watch?v=R-T3Mk5oDHI&t=2389s (38:51)
/// @todo: It (should) control:
// - Where to start the target state from (match event, percentage, etc)
// - How to step the time for both states (sync)
// - How to blend (or not) the root motion during the transition
class TransitionRuntimeNode : public PoseRuntimeNode
{
  public:
    struct Settings : public PoseRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;
        friend class StateMachineEditorNode;
        friend class TransitionEditorNode;

      private:
        NodeIndex m_targetStateNodeIdx = InvalidIndex;
        float m_duration = 0.0f; // How long a blend between two states will take (in seconds)

      public:
        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<TransitionRuntimeNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_targetStateNodeIdx, pNode->m_pTargetNode);
        }
    };

  private:
    PoseRuntimeNode* m_pSourceNode = nullptr;
    StateRuntimeNode* m_pTargetNode = nullptr;
    float m_duration = 0.0f;
    float m_transitionProgress = 0.0f;

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