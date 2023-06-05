#pragma once

#include "../pose.hpp"
#include "../types.hpp"
#include "pose_buffer_pool.hpp"

#include <common/update_stages.hpp>

#include <vector>

namespace aln
{

class Task;

struct TaskContext
{
    PoseBufferPool* m_pPoseBufferPool = nullptr;
    std::vector<Task*> m_dependencies;
    Percentage m_deltaTime = 0.0f;
    Transform m_worldTransform = Transform::Identity;

    TaskContext(PoseBufferPool* pPoseBufferPool) : m_pPoseBufferPool(pPoseBufferPool) {}
};

/// @brief Atomic pose operation, creating one or operating on one.
/// @todo: Some tasks may have physics dependency (pre-post physics)
class Task
{
    friend class TaskSystem;

  private:
    TaskIndex m_index = InvalidIndex;
    bool m_completed = false;
    NodeIndex m_sourceNodeIdx = InvalidIndex;
    PoseBufferIndex m_resultBufferIndex = InvalidIndex;
    std::vector<TaskIndex> m_dependencies;

    UpdateStage m_updateStage;

    // Disable copies
    Task(const Task& rhs) = delete;
    Task& operator=(const Task& rhs) = delete;

  protected:
    /// @brief Get an unused pose buffer
    PoseBuffer* GetNewPoseBuffer(const TaskContext& context)
    {
        m_resultBufferIndex = context.m_pPoseBufferPool->GetFirstAvailableBufferIndex();
        auto pBuffer = context.m_pPoseBufferPool->GetByIndex(m_resultBufferIndex);
        pBuffer->m_owner = m_index;
        return pBuffer;
    }

    /// @brief Release the currently held buffer
    void ReleasePoseBuffer(const TaskContext& context)
    {
        assert(m_resultBufferIndex != InvalidIndex);
        context.m_pPoseBufferPool->ReleasePoseBuffer(m_resultBufferIndex);
        m_resultBufferIndex = InvalidIndex;
    }

    /// @brief Release the buffer held by one of this task's dependencies
    void ReleaseDependencyPoseBuffer(const TaskContext& context, PoseBufferIndex dependencyIndex)
    {
        assert(dependencyIndex < context.m_dependencies.size());
        auto pDependency = context.m_dependencies[dependencyIndex];

        assert(pDependency != nullptr && pDependency->IsComplete() && pDependency->m_resultBufferIndex != InvalidIndex);
        pDependency->ReleasePoseBuffer(context);
    }

    /// @brief Retrieve the output pose buffer of a depency and acquire its ownership
    PoseBuffer* TransferDependencyPoseBuffer(const TaskContext& context, uint8_t dependencyIndex)
    {
        assert(dependencyIndex < context.m_dependencies.size());
        auto pDependency = context.m_dependencies[dependencyIndex];

        assert(pDependency != nullptr && pDependency->IsComplete() && pDependency->m_resultBufferIndex != InvalidIndex);
        m_resultBufferIndex = pDependency->GetResultBufferIndex();
        pDependency->m_resultBufferIndex = InvalidIndex;

        auto pBuffer = context.m_pPoseBufferPool->GetByIndex(m_resultBufferIndex);
        pBuffer->m_owner = m_index;

        return pBuffer;
    }

    /// @brief Retrieve the ouptut pose buffer of a dependency
    PoseBuffer* AccessDependencyPoseBuffer(const TaskContext& context, uint8_t dependencyIndex)
    {
        assert(dependencyIndex < context.m_dependencies.size());
        auto pDependency = context.m_dependencies[dependencyIndex];

        assert(pDependency != nullptr && pDependency->IsComplete() && pDependency->m_resultBufferIndex != InvalidIndex);

        return context.m_pPoseBufferPool->GetByIndex(pDependency->GetResultBufferIndex());
    }

    void MarkTaskComplete(const TaskContext& context)
    {
        assert(!m_completed);
        m_completed = true;
    }

  public:
    Task(NodeIndex sourceNodeIdx) : m_sourceNodeIdx(sourceNodeIdx) {}
    Task(NodeIndex sourceNodeIdx, UpdateStage updateStage, std::vector<TaskIndex> dependencies)
        : m_sourceNodeIdx(sourceNodeIdx), m_updateStage(updateStage), m_dependencies(dependencies) {}

    bool IsComplete() const { return m_completed; }
    PoseBufferIndex GetResultBufferIndex() const { return m_resultBufferIndex; }

    bool HasDependencies() const { return !m_dependencies.empty(); }
    const std::vector<TaskIndex>& GetDependencies() const { return m_dependencies; }

    virtual void Execute(const TaskContext& context) = 0;
};
} // namespace aln