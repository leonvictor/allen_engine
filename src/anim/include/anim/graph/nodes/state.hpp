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
  public:
    struct Settings : public PassthroughRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;
        friend class StateEditorNode;

      private:
      public:
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

  private:
    void StartTransitionFrom(GraphContext& context)
    {
        assert(false);
        // TODO
    }

    void StartTransitionTo(GraphContext& context)
    {
        assert(false);
        // TODO
    }

  public:
    bool IsTransitioning() const { return m_transitionState != TransitionState::None; }
    bool IsTransitioningOut() const { return m_transitionState == TransitionState::TransitionOutgoing; }
    bool IsTransitioningIn() const { return m_transitionState == TransitionState::TransitionIncoming; }

    PoseNodeResult Update(GraphContext& context) override
    {
        auto result = PassthroughRuntimeNode::Update(context);
        // TODO: Track time spent in current state
        // TODO: Sample events
        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        auto result = PassthroughRuntimeNode::Update(context, updateRange);
        // TODO: Track time spent in current state
        // TODO: Sample events
        return result;
    }

    void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        PassthroughRuntimeNode::Initialize(context, initialTime);
        // TODO
    }

    void ShutdownInternal() override
    {
        // TODO
        PassthroughRuntimeNode::Shutdown();
    }
};
} // namespace aln