#pragma once

#include "../pose_node.hpp"
#include "../value_node.hpp"
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
        BoolValueNode* m_pConditionNode = nullptr;
        uint16_t m_endStateIndex = InvalidIndex; // Index of the end state in the node's states array
    };

    /// @brief Holds a ptr to a state node and info about available transitions from it
    struct State
    {
        StateRuntimeNode* m_pStateNode = nullptr;
        Vector<Transition> m_transitions;
    };

    struct Settings : public PoseRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;
        friend class StateMachineEditorNode;

        struct TransitionSettings
        {
            uint32_t m_endStateIndex = InvalidIndex; // Index of the end state in the node's states array
            NodeIndex m_transitionNodeIndex = InvalidIndex;
            NodeIndex m_conditionNodeIndex = InvalidIndex;
        };

        struct StateSettings
        {
            NodeIndex m_stateNodeIndex = InvalidIndex;
            Vector<TransitionSettings> m_transitionSettings;

            template<class Archive>
            void Serialize(Archive& archive) const
            {
                archive << m_stateNodeIndex;
                archive << m_transitionSettings;
            }

            template <class Archive>
            void Deserialize(Archive& archive)
            {
                archive >> m_stateNodeIndex;
                archive >> m_transitionSettings;
            }
        };

      private:
        Vector<StateSettings> m_stateSettings;

      public:
        virtual void InstanciateNode(const Vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<StateMachineRuntimeNode>(nodePtrs, options);

            for (auto& stateSettings : m_stateSettings)
            {
                auto& state = pNode->m_states.emplace_back();
                SetNodePtrFromIndex(nodePtrs, stateSettings.m_stateNodeIndex, state.m_pStateNode);
                for (auto& transitionSettings : stateSettings.m_transitionSettings)
                {
                    auto& transition = state.m_transitions.emplace_back();
                    transition.m_endStateIndex = transitionSettings.m_endStateIndex;
                    SetNodePtrFromIndex(nodePtrs, transitionSettings.m_transitionNodeIndex, transition.m_pTransitionNode);
                    SetNodePtrFromIndex(nodePtrs, transitionSettings.m_conditionNodeIndex, transition.m_pConditionNode);
                }
            }
        }
    };

  private:
    Vector<State> m_states;
    TransitionRuntimeNode* m_pActiveTransitionNode = nullptr;
    uint16_t m_activeStateIndex = InvalidIndex;

    const State& GetActiveState() const
    {
        assert(m_activeStateIndex != InvalidIndex);
        return m_states[m_activeStateIndex];
    }

    const Vector<Transition>& GetAvailableTransitionsFromActiveState() const
    {
        assert(m_activeStateIndex != InvalidIndex);
        return m_states[m_activeStateIndex].m_transitions;
    }

  public:
    bool IsTransitionActive() const { return m_pActiveTransitionNode != nullptr; }

    PoseNodeResult Update(GraphContext& context) override
    {
        assert(m_activeStateIndex != InvalidIndex);

        auto pActiveStateNode = GetActiveState().m_pStateNode;

        // Shutdown completed transitions
        if (IsTransitionActive() && m_pActiveTransitionNode->TransitionComplete())
        {
            m_pActiveTransitionNode->Shutdown();
            m_pActiveTransitionNode = nullptr;
        }

        // Update current transition if one is active, otherwise update current state
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
            result = pActiveStateNode->Update(context);
            m_duration = pActiveStateNode->GetDuration();
            m_currentTime = pActiveStateNode->GetCurrentTime();
            m_previousTime = pActiveStateNode->GetPreviousTime();
        }

        // Find if a new transition should be started
        uint32_t newTransitionIdx = InvalidIndex;
        const auto& availableTransitions = GetAvailableTransitionsFromActiveState();
        const auto availableTransitionsCount = availableTransitions.size();
        for (auto transitionIdx = 0; transitionIdx < availableTransitionsCount; ++transitionIdx)
        {
            const auto& transition = availableTransitions[transitionIdx];
            if (transition.m_pConditionNode->GetValue<bool>(context))
            {
                if (m_pActiveTransitionNode != nullptr)
                {
                    assert(false); // TODO: Whats happens when a transition is triggered while the previous one is not finished ?
                }

                newTransitionIdx = transitionIdx;
                break;
            }
        }

        // Start the new transition
        if (newTransitionIdx != InvalidIndex)
        {
            // Shutdown the previous state's outgoing transitions' condition nodes
            for (auto& previouslyAvailableTransition : m_states[m_activeStateIndex].m_transitions)
            {
                if (previouslyAvailableTransition.m_pConditionNode != nullptr)
                {
                    previouslyAvailableTransition.m_pConditionNode->Shutdown();
                }
            }

            // Set active transition
            const auto& transition = GetAvailableTransitionsFromActiveState()[newTransitionIdx];
            m_pActiveTransitionNode = transition.m_pTransitionNode;
            m_pActiveTransitionNode->StartFrom(context, pActiveStateNode, result);

            // Switch active state
            m_activeStateIndex = transition.m_endStateIndex;

            // Initialize the next state's outgoing transitions' condition nodes
            for (auto& newlyAvailableTransition : m_states[m_activeStateIndex].m_transitions)
            {
                if (newlyAvailableTransition.m_pConditionNode != nullptr)
                {
                    newlyAvailableTransition.m_pConditionNode->Initialize(context);
                }
            }

            // Update internal state to that of the target node
            auto pNewState = GetActiveState().m_pStateNode;
            m_duration = pNewState->GetDuration();
            m_currentTime = pNewState->GetCurrentTime();
            m_previousTime = pNewState->GetPreviousTime();
        }

        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        // TODO
        PoseNodeResult result;
        assert(false); 
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