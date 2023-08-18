#pragma once

#include "../passthrough_node.hpp"

namespace aln
{
/// @brief
// @todo State events = StringIDs emitted from states that can be used by gameplay code to react
// 4 types of state events: TransitioningIn, transitioningOut, FullyInState, Timed (elapsed time in state > or < than... - remaining time in state >< that...)
// TODO: State events can also be checked from within the graph (entry state overrides or transitions)
class StateRuntimeNode : public PassthroughRuntimeNode
{
    friend class TransitionRuntimeNode;

  public:
    struct Settings : public PassthroughRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;
        friend class StateEditorNode;

        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<StateRuntimeNode>(nodePtrs, options);
            PassthroughRuntimeNode::Settings::InstanciateNode(nodePtrs, pDataSet, options);
        }
    };

  private:
    enum class TransitionState : uint8_t
    {
        None,
        TransitionOutgoing,
        TransitionIncoming,
    };

  private:
    TransitionState m_transitionState = TransitionState::None;
    float m_timeSpentInState = 0.0f;

  private:
    void StartTransitioningFrom(GraphContext& context)
    {
        m_transitionState = TransitionState::TransitionOutgoing;
    }

    void StartTransitioningTo(GraphContext& context)
    {
        m_transitionState = TransitionState::TransitionIncoming;
    }

  public:
    bool IsTransitioning() const { return m_transitionState != TransitionState::None; }
    bool IsTransitioningOut() const { return m_transitionState == TransitionState::TransitionOutgoing; }
    bool IsTransitioningIn() const { return m_transitionState == TransitionState::TransitionIncoming; }

    PoseNodeResult Update(GraphContext& context) override
    {
        auto result = PassthroughRuntimeNode::Update(context);
        m_timeSpentInState += context.m_deltaTime;

        // TODO: Sample events

        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        auto result = PassthroughRuntimeNode::Update(context, updateRange);
        m_timeSpentInState += context.m_deltaTime;

        // TODO: Sample events

        return result;
    }

    void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        PassthroughRuntimeNode::Initialize(context, initialTime);
        m_transitionState = TransitionState::None;
        m_timeSpentInState = 0.0f;
    }

    void ShutdownInternal() override
    {
        m_timeSpentInState = 0.0f;
        m_transitionState = TransitionState::None;
        PassthroughRuntimeNode::Shutdown();
    }
};
} // namespace aln