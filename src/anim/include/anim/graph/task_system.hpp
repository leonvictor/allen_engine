#pragma once

#include "../pose.hpp"
#include "../types.hpp"
#include "pose_buffer_pool.hpp"
#include "task.hpp"

#include <common/containers/vector.hpp>

namespace aln
{

class TaskSystem
{
  private:
    PoseBufferPool m_poseBufferPool;

    Vector<Task*> m_registeredTasks;
    TaskContext m_taskContext;

  public:
    TaskSystem(const Skeleton* pSkeleton) : m_poseBufferPool(pSkeleton), m_taskContext(&m_poseBufferPool) {}

    ~TaskSystem()
    {
        Reset();
    }

    void ExecuteTasks(float deltaTime, const Transform& worldTransform, Pose* pOutPose)
    {
        m_taskContext.m_deltaTime = deltaTime;

        const auto taskCount = m_registeredTasks.size();
        for (auto taskIndex = 0; taskIndex < taskCount; ++taskIndex)
        {
            m_taskContext.m_dependencies.clear();
            for (auto& dependencyIndex : m_registeredTasks[taskIndex]->GetDependencies())
            {
                m_taskContext.m_dependencies.emplace_back(m_registeredTasks[dependencyIndex]);
            }
            m_registeredTasks[taskIndex]->Execute(m_taskContext);
        }

        auto pLastTask = m_registeredTasks.back();
        auto pResultBuffer = m_poseBufferPool.GetByIndex(pLastTask->GetResultBufferIndex());

        pOutPose->CopyFrom(pResultBuffer->m_pose);
        pOutPose->CalculateGlobalTransforms();

        m_poseBufferPool.ReleasePoseBuffer(pLastTask->GetResultBufferIndex());
    }

    void Reset()
    {
        for (auto pTask : m_registeredTasks)
        {
            aln::Delete(pTask);
        }
        m_registeredTasks.clear();
    }

    /// @brief Tasks are registered by each node during their update loop
    template <typename TaskType, typename... ConstructorParams>
    TaskIndex RegisterTask(ConstructorParams&&... params)
    {
        auto pTask = m_registeredTasks.emplace_back(aln::New<TaskType>(std::forward<ConstructorParams>(params)...));
        pTask->m_index = m_registeredTasks.size() - 1;
        return pTask->m_index;
    }
};
} // namespace aln