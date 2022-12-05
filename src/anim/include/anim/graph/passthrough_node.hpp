#pragma once

#include "pose_node.hpp"

namespace aln
{

// TODO
class PassthroughRuntimeNode : public PoseRuntimeNode
{
  private:
    std::vector<PoseRuntimeNode*> m_pChildNodes; // todo

  protected:
    PoseNodeResult Update(GraphContext& context) override
    {
        // TODO: Where does task registration happen ?
        // TODO: We should provide dependencies on children here ?
        PoseNodeResult result;
        std::vector<TaskIndex> wouldBeTaskDependencies;
        for (auto& pChild : m_pChildNodes)
        {
            // TODO: Populate dependencies
            auto childResult = pChild->Update(context);
            if (childResult.HasRegisteredTasks())
            {
                wouldBeTaskDependencies.push_back(childResult.m_taskIndex);
            }
        }

        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        PoseNodeResult result;
        for (auto& pChild : m_pChildNodes)
        {
            pChild->Update(context, updateRange);
        }
        return result;
    }

  public:
    virtual const SyncTrack& GetSyncTrack() override const {
        // TODO: Abstract method impl ?
    };

    bool IsChildValid(const size_t index = 0) const
    {
        assert(index < m_pChildNodes.size());
        return m_pChildNodes[index]->IsValid();
    }
};
} // namespace aln