#pragma once

#include "pose_node.hpp"

namespace aln
{

// TODO
class PassthroughNode : public PoseNode
{
  protected:
    struct Settings : public PoseNode::Settings
    {
    };

    PoseNode* m_pChildNode; // todo

    PoseNodeResult Update(GraphContext& context) override
    {
        return m_pChildNode->Update(context);
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        return m_pChildNode->Update(context, updateRange);
    }

  public:
    bool IsChildValid() const; // todo
};
} // namespace aln