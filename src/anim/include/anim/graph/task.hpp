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
    // Buffer indices for the dependencies output
    std::vector<PoseBufferIndex> m_dependencyBufferIndices;
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
    NodeIndex m_sourceNodeIdx = InvalidIndex;
    PoseBufferIndex m_resultBufferIndex = InvalidIndex;
    std::vector<TaskIndex> m_dependencies;

     UpdateStage m_updateStage;

    // Disable copies
    Task(const Task& rhs) = delete;
    Task& operator=(const Task& rhs) = delete;

  protected:
    // TODO: bufferindex ?
    /// @brief Release the buffer held by one of this task's dependencies
    /// @param context: Todo
    /// @param dependency: Index of the dependency
    void ReleaseDependencyPoseBuffer(const TaskContext& context, PoseBufferIndex dependencyIndex)
    {
        // TODO: How do we access the taskSystem which holds the buffers ?
        auto pBuffer = context.m_pPoseBufferPool->GetByIndex(context.m_dependencyBufferIndices[dependencyIndex]);
        // TODO: Reset pose ?
        pBuffer->m_owner = InvalidIndex;
    }

    /// @brief Retrieve the output pose buffer of a depency and transfer ownership to self
    PoseBuffer* TransferDependencyPoseBuffer(const TaskContext& context, uint8_t dependencyIndex)
    {
        // TODO
        auto pBuffer = context.m_pPoseBufferPool->GetByIndex(context.m_dependencyBufferIndices[dependencyIndex]);
        pBuffer->m_owner = m_index;
        return pBuffer;
    }

    /// @brief Retrieve the ouptut pose buffer of a dependency
    PoseBuffer* AccessDependencyPoseBuffer(const TaskContext& context, uint8_t dependencyIndex)
    {
        // TODO
        return context.m_pPoseBufferPool->GetByIndex(context.m_dependencyBufferIndices[dependencyIndex]);
    }

    /// @brief Get an unused pose buffer
    PoseBuffer* GetNewPoseBuffer(const TaskContext& context)
    {
        m_resultBufferIndex = context.m_pPoseBufferPool->GetFirstAvailableBufferIndex();
        auto pBuffer = context.m_pPoseBufferPool->GetByIndex(m_resultBufferIndex);
        pBuffer->m_owner = m_index;
        return pBuffer;
    }

    void MarkTaskComplete(const TaskContext& context)
    {
        // TODO
    }

  public:
    Task(NodeIndex sourceNodeIdx) : m_sourceNodeIdx(sourceNodeIdx) {}
    Task(NodeIndex sourceNodeIdx, UpdateStage updateStage, std::vector<TaskIndex> dependencies)
        : m_sourceNodeIdx(sourceNodeIdx), m_updateStage(updateStage), m_dependencies(dependencies) {}

    PoseBufferIndex GetResultBufferIndex() const { return m_resultBufferIndex; }

    bool HasDependencies() const { return !m_dependencies.empty(); }
    const std::vector<TaskIndex>& GetDependencies() const { return m_dependencies; }

    virtual void Execute(const TaskContext& context) = 0;
};
} // namespace aln