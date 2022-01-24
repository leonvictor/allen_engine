#pragma once

#include <cstdint>
#include <vector>

#include "../../pose.hpp"

namespace aln
{

struct TaskContext
{
};

using TaskIndex = uint32_t;
static const TaskIndex InvalidIndex = UINT32_MAX;

class Task
{
  private:
    NodeIndex m_sourceNodeIdx = InvalidIndex;
    std::vector<TaskIndex> m_dependencies;
    UpdateStage m_updateStage;

  protected:
    // TODO: bufferindex ?
    void ReleaseDependencyPoseBuffer(const TaskContext& context, uint8_t bufferIndex)
    {
        // TODO
    }

    PoseBuffer* TransferDependencyPoseBuffer(const TaskContext& context, uint8_t bufferIndex)
    {
        // TODO
    }

    PoseBuffer* AccessDependencyPoseBuffer(const TaskContext& context, uint8_t bufferIndex)
    {
        // TODO
    }

    PoseBuffer* GetNewPoseBuffer(const TaskContext& context)
    {
        // TODO
    }

    void MarkTaskComplete(const TaskContext& context)
    {
        // TODO
    }

  public:
    Task(NodeIndex sourceNodeIdx) : m_sourceNodeIdx(sourceNodeIdx) {}
    Task(NodeIndex sourceNodeIdx, UpdateStage updateStage, std::vector<TaskIndex> dependencies) : m_sourceNodeIdx(sourceNodeIdx), m_updateStage(updateStage), m_dependencies(dependencies) {}

    virtual void Execute(const TaskContext& context) = 0;
};
} // namespace aln