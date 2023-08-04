#pragma once

#include "../pose_node.hpp"
#include "state.hpp"
#include "transition.hpp"

namespace aln
{

class StateMachineRuntimeNode : public PoseRuntimeNode
{
  public:
    struct Transition
    {
        TransitionRuntimeNode* m_pTransitionNode = nullptr;
        uint16_t m_targetStateIndex = InvalidIndex; // Index of the target state in the node's states array
    };

    /// @brief Holds a ptr to a state node and info about available transitions from it
    struct State
    {
        StateRuntimeNode* m_pStateNode = nullptr;
        std::vector<Transition> m_transitions;
    };

    struct Settings : public PoseRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;

        struct TransitionSettings
        {
            NodeIndex m_transitionNodeIndex = InvalidIndex;
            uint16_t m_targetStateIndex = InvalidIndex; // Index of the target state in the node's states array
        };

        struct StateSettings
        {
            NodeIndex m_stateNodeIndex = InvalidIndex;
            std::vector<TransitionSettings> m_transitionSettings;
        };

      private:
        std::vector<StateSettings> m_stateSettings;

      public:
        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<StateMachineRuntimeNode>(nodePtrs, options);

            for (auto& stateSettings : m_stateSettings)
            {
                auto& state = pNode->m_states.emplace_back();
                SetNodePtrFromIndex(nodePtrs, stateSettings.m_stateNodeIndex, state.m_pStateNode);

                for (auto& transitionSettings : stateSettings.m_transitionSettings)
                {
                    auto& transition = state.m_transitions.emplace_back();
                    transition.m_targetStateIndex = transitionSettings.m_targetStateIndex;
                    SetNodePtrFromIndex(nodePtrs, transitionSettings.m_transitionNodeIndex, transition.m_pTransitionNode);
                }
            }
        }
    };

  private:
    std::vector<State> m_states;
    TransitionRuntimeNode* m_pActiveTransitionNode = nullptr;
    uint16_t m_activeStateIndex = InvalidIndex;

    const State& GetActiveState() const
    {
        assert(m_activeStateIndex != InvalidIndex);
        return m_states[m_activeStateIndex];
    }

    const std::vector<Transition>& GetAvailableTransitionsFromActiveState() const
    {
        assert(m_activeStateIndex != InvalidIndex);
        return m_states[m_activeStateIndex].m_transitions;
    }

  public:
    bool IsTransitionActive() const { return m_pActiveTransitionNode != nullptr; }

    PoseNodeResult Update(GraphContext& context) override
    {
        assert(m_activeStateIndex != InvalidIndex);

        // Shutdown completed transitions
        if (IsTransitionActive() && m_pActiveTransitionNode->TransitionComplete())
        {
            m_pActiveTransitionNode->Shutdown();
            m_pActiveTransitionNode = nullptr;
        }

        PoseNodeResult result;
        if (IsTransitionActive())
        {
            result = m_pActiveTransitionNode->Update(context);
            m_duration = m_pActiveTransitionNode->GetDuration();
            m_currentTime = m_pActiveTransitionNode->GetCurrentTime();
            m_previousTime = m_pActiveTransitionNode->GetPreviousTime();
        }
        else
        {
            auto pActiveStateNode = GetActiveState().m_pStateNode;

            result = pActiveStateNode->Update(context);
            m_duration = pActiveStateNode->GetDuration();
            m_currentTime = pActiveStateNode->GetCurrentTime();
            m_previousTime = pActiveStateNode->GetPreviousTime();
        }

        // TODO: Start transitions if necessary

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
        PoseRuntimeNode::InitializeInternal(context, initialTime);

        // TODO: Allow users to override the default state through a setting
        m_activeStateIndex = 0;
        auto pActiveStateNode = GetActiveState().m_pStateNode;
        pActiveStateNode->Initialize(context, initialTime);

        m_duration = pActiveStateNode->GetDuration();
        m_currentTime = pActiveStateNode->GetCurrentTime();
        m_previousTime = pActiveStateNode->GetPreviousTime();
    }

    void ShutdownInternal() override
    {
        if (IsTransitionActive())
        {
            m_pActiveTransitionNode->Shutdown();
            m_pActiveTransitionNode = nullptr;
        }

        GetActiveState().m_pStateNode->Shutdown();
        m_activeStateIndex = InvalidIndex;

        PoseRuntimeNode::ShutdownInternal();
    }

    const SyncTrack& GetSyncTrack() const override
    {
        if (IsTransitionActive())
        {
            return m_pActiveTransitionNode->GetSyncTrack();
        }
        else
        {
            return GetActiveState().m_pStateNode->GetSyncTrack();
        }
    }
};
} // namespace aln