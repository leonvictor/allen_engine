#pragma once

#include "../../blender.hpp"
#include "../pose_node.hpp"
#include "../tasks/blend_task.hpp"
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
    // TODO: Transition can be marked "synchronized"
    struct Settings : public PoseRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;
        friend class StateMachineEditorNode;
        friend class TransitionEditorNode;
        friend class TransitionRuntimeNode;

      private:
        NodeIndex m_endStateNodeIdx = InvalidIndex;
        float m_transitionDuration = 0.0f; // How long a blend between two states will take (in seconds)

      public:
        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<TransitionRuntimeNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_endStateNodeIdx, pNode->m_pEndNode);
        }
    };

  private:
    StateRuntimeNode* m_pStartNode = nullptr;
    StateRuntimeNode* m_pEndNode = nullptr;
    float m_transitionDuration = 0.0f; // How long a blend between two states will take (in seconds)
    float m_transitionProgress = 0.0f; // [0, 1]

  public:
    bool TransitionComplete() const { return m_transitionProgress >= 1.0f; }

    PoseNodeResult Update(GraphContext& context) override
    {
        assert(m_pStartNode != nullptr && m_pStartNode->IsInitialized() && m_pEndNode != nullptr && m_pEndNode->IsInitialized());
        assert(IsInitialized());

        m_transitionProgress += context.m_deltaTime / m_transitionDuration;
        m_transitionProgress = std::clamp(m_transitionProgress, 0.0f, 1.0f);

        m_duration = glm::lerp(m_pStartNode->GetDuration(), m_pEndNode->GetDuration(), m_transitionProgress);

        // Update start node
        PoseNodeResult startNodeResult = m_pStartNode->Update(context);
        assert(startNodeResult.HasRegisteredTasks()); // TODO: Handle the case !

        // Update end node
        PoseNodeResult endNodeResult = m_pEndNode->Update(context);
        assert(endNodeResult.HasRegisteredTasks()); // TODO: Handle the case !

        // Update the transition itself
        PoseNodeResult result;
        BitFlags<PoseBlend> blendOptions; // TODO
        result.m_taskIndex = context.m_pTaskSystem->RegisterTask<BlendTask>(GetNodeIndex(), startNodeResult.m_taskIndex, endNodeResult.m_taskIndex, m_transitionProgress, blendOptions, nullptr);
        // TODO: we could skip an op by not interpolating scale (which is not used by the root motion track)
        result.m_rootMotionDelta = Transform::Interpolate(startNodeResult.m_rootMotionDelta, endNodeResult.m_rootMotionDelta, m_transitionProgress);

        // Update internal time
        m_previousTime = m_currentTime;
        m_currentTime = m_currentTime + (context.m_deltaTime / m_duration);

        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        assert(m_pStartNode != nullptr && m_pStartNode->IsInitialized() && m_pEndNode != nullptr && m_pEndNode->IsInitialized());
        assert(IsInitialized());

        m_transitionProgress += context.m_deltaTime / m_transitionDuration;
        m_transitionProgress = std::clamp(m_transitionProgress, 0.0f, 1.0f);

        m_duration = glm::lerp(m_pStartNode->GetDuration(), m_pEndNode->GetDuration(), m_transitionProgress);
        
        PoseNodeResult result;

        PoseNodeResult startNodeResult = m_pStartNode->Update(context, updateRange);
        assert(startNodeResult.HasRegisteredTasks());

        assert(false); // TODO
        return result;
    }

    void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        PoseRuntimeNode::InitializeInternal(context, initialTime);

        const auto pSettings = GetSettings<TransitionRuntimeNode>();
        m_transitionDuration = pSettings->m_transitionDuration; // TODO: Optionnaly override through a node
        m_transitionProgress = 0.0f;
    }

    void ShutdownInternal() override
    {
        m_transitionDuration = 0.0f;
        m_transitionProgress = 0.0f;
        PoseRuntimeNode::ShutdownInternal();
    }

    const SyncTrack& GetSyncTrack() const override
    {
        if (TransitionComplete())
        {
            return m_pEndNode->GetSyncTrack();
        }
        else
        {
            return SyncTrack::Default; // TODO: ?
        }
    }

    PoseNodeResult StartFrom(GraphContext& context, StateRuntimeNode* pStartState, PoseNodeResult& startNodeResult)
    {
        assert(pStartState != nullptr);

        // Initialize transition node
        PoseRuntimeNode::Initialize(context, SyncTrackTime());

        pStartState->StartTransitioningFrom(context);
        m_pStartNode = pStartState;
        m_currentTime = 0.0f;

        // Initialize end state node
        m_pEndNode->Initialize(context, SyncTrackTime());
        m_pEndNode->StartTransitioningTo(context);
        auto endNodeResult = m_pEndNode->Update(context);

        // Immediately update
        PoseNodeResult result;
        // TODO: Synchronized / Unsynchronized
        const auto pSettings = GetSettings<TransitionRuntimeNode>();

        m_transitionProgress += context.m_deltaTime / m_transitionDuration;
        m_transitionProgress = std::clamp(m_transitionProgress, 0.0f, 1.0f);

        // TODO : Handle other easing functions
        const auto blendWeight = m_transitionProgress;
        if (startNodeResult.HasRegisteredTasks() && endNodeResult.HasRegisteredTasks())
        {
            BitFlags<PoseBlend> blendOptions; // TODO
            result.m_taskIndex = context.m_pTaskSystem->RegisterTask<BlendTask>(GetNodeIndex(), startNodeResult.m_taskIndex, endNodeResult.m_taskIndex, blendWeight, blendOptions, nullptr);
            // TODO: we could skip an op by not interpolating scale (which is not used by the root motion track)
            result.m_rootMotionDelta = Transform::Interpolate(startNodeResult.m_rootMotionDelta, endNodeResult.m_rootMotionDelta, blendWeight);
        }
        else
        {
            if (startNodeResult.HasRegisteredTasks())
            {
                result.m_taskIndex = startNodeResult.m_taskIndex;
            }
            else if (endNodeResult.HasRegisteredTasks())
            {
                result.m_taskIndex = endNodeResult.m_taskIndex;
            }
            else
            {
                assert(false);
            }
        }

        return result;
    }
};
} // namespace aln