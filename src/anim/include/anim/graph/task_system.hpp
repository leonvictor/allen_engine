#pragma once

#include "../pose.hpp"
#include "../types.hpp"
#include "pose_buffer_pool.hpp"
#include "task.hpp"

#include <vector>

namespace aln
{

// TODO
class TaskSystem
{
  private:
    // TODO:
    // PoseBufferPool m_poseBufferPool;

    std::vector<Task> m_registeredTasks;

  public:
    TaskSystem()
    {
    }

    TaskIndex RegisterTask()
    {
        // TODO: When is this called ?
    }
};
} // namespace aln