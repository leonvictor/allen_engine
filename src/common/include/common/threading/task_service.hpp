#pragma once

#include <TaskScheduler.h>
#include <cstdint>

#include "../services/service.hpp"

namespace aln
{

using TaskSet = enki::TaskSet;
using ITaskSet = enki::ITaskSet;
using TaskSetFunction = enki::TaskSetFunction;
using TaskSetPartition = enki::TaskSetPartition;

class TaskService : public IService
{

  private:
    uint32_t m_numWorkers;

    enki::TaskScheduler m_taskScheduler;

    static void* CustomAllocFunc(size_t alignment, size_t size, void* userData, const char* file, int line)
    {
        return aln::Allocate(size, alignment);
    }

    static void CustomFreeFunc(void* ptr, size_t size, void* userData, const char* file, int line)
    {
        aln::Free(ptr);
    }

  public:
    TaskService() : m_numWorkers(std::thread::hardware_concurrency())
    {
        enki::TaskSchedulerConfig config;
        config.numTaskThreadsToCreate = m_numWorkers; // TODO: -1 to exclude main thread ?
        config.customAllocator.alloc = CustomAllocFunc;
        config.customAllocator.free = CustomFreeFunc;

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