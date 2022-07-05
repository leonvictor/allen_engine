#pragma once

#include <TaskScheduler.h>
#include <cstdint>

namespace aln
{

using TaskSet = enki::TaskSet;
using ITaskSet = enki::ITaskSet;
using TaskSetPartition = enki::TaskSetPartition;

class TaskService
{

  private:
    uint32_t m_numWorkers;

    enki::TaskScheduler m_taskScheduler;

  public:
    TaskService() : m_numWorkers(std::thread::hardware_concurrency())
    {
        enki::TaskSchedulerConfig config;
        config.numTaskThreadsToCreate = m_numWorkers; // TODO: -1 to exclude main thread ?

        m_taskScheduler.Initialize(config);
    }

    void ScheduleTask(ITaskSet* pTask)
    {
        m_taskScheduler.AddTaskSetToPipe(pTask);
    }

    void WaitForTask(ITaskSet* pTask)
    {
        m_taskScheduler.WaitforTaskSet(pTask);
    }

    /// @brief Execute a parallel task and wait for the result
    void ExecuteTask(ITaskSet* pTask)
    {
        ScheduleTask(pTask);
        WaitForTask(pTask);
    }
};
} // namespace aln