#pragma once

#include "../pose.hpp"
#include "pose_buffer_pool.hpp"
#include "task.hpp"
#include "types.hpp"

#include <vector>

namespace aln
{

// TODO
class TaskSystem
{
  private:
    // TODO: Tasks will request, free and transfer pose buffers from the pool
    std::vector<PoseBuffer> m_poseBufferPool;
    // TODO: Pool dynamically grow if more poses are requested

    std::vector<Task> m_registeredTasks;

  public:
    TaskSystem()
    {
        // avg size = ~5 poses per pool
        m_poseBufferPool.resize(5);
    }

    TaskIndex RegisterTask()
    {
        // TODO: When is this called ?
    }
};
} // namespace aln