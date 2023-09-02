#pragma once

#include "pose_node.hpp"

namespace aln
{

class PassthroughRuntimeNode : public PoseRuntimeNode
{
  public:
    class Settings : public PoseRuntimeNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;
        friend class StateEditorNode; // TODO: Giving access through friend everytime there's a new passthrough node is not goog. Make it public ?

      private:
        NodeIndex m_childNodeIdx = InvalidIndex;

      public:
        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            // TODO: Make sure the node has already been created
            auto pNode = reinterpret_cast<PassthroughRuntimeNode*>(nodePtrs[GetNodeIndex()]);
            SetNodePtrFromIndex(nodePtrs, m_childNodeIdx, pNode->m_pChildNode);
        }
    };

  private:
    PoseRuntimeNode* m_pChildNode = nullptr;

  protected:
    PoseNodeResult Update(GraphContext& context) override
    {
        assert(m_pChildNode != nullptr);

        auto result = m_pChildNode->Update(context);

        // Update internal time to match child's
        m_duration = m_pChildNode->GetDuration();
        m_previousTime = m_pChildNode->GetPreviousTime();
        m_currentTime = m_pChildNode->GetCurrentTime();

        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        assert(m_pChildNode != nullptr);

        auto result = m_pChildNode->Update(context, updateRange);

        // Update internal time to match child's
        m_duration = m_pChildNode->GetDuration();
        m_previousTime = m_pChildNode->GetPreviousTime();
        m_currentTime = m_pChildNode->GetCurrentTime();

        return result;
    }

    void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime) override
    {
        PoseRuntimeNode::InitializeInternal(context, initialTime);

        assert(m_pChildNode != nullptr);
        m_duration = m_pChildNode->GetDuration();
        m_previousTime = m_pChildNode->GetPreviousTime();
        m_currentTime = m_pChildNode->GetCurrentTime();
    }

    void ShutdownInternal() override
    {
        assert(m_pChildNode != nullptr);

        m_pChildNode->Shutdown();
        PoseRuntimeNode::ShutdownInternal();
    }

  public:
    virtual const SyncTrack& GetSyncTrack() const override
    {
        assert(m_pChildNode != nullptr);
        return m_pChildNode->GetSyncTrack();
    };
};
} // namespace aln