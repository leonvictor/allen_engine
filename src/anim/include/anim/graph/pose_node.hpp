#pragma once

#include <assert.h>
#include <vector>

#include <common/transform.hpp>

#include "../event.hpp"
#include "../sync_track.hpp"
#include "graph_node.hpp"
#include "task.hpp"

namespace aln
{
using Seconds = float;
using Percentage = float;

struct PoseTask
{
    std::vector<TaskIndex> m_dependencies;
};

struct SampledEventRange
{
};

struct PoseNodeResult
{
    uint32_t m_taskIdx; // todo

  public:
    inline bool HasRegisteredTasks() const { return m_taskIdx != InvalidIndex; }

    TaskIndex m_taskIndex = InvalidIndex;
    Transform m_rootMotionDelta = Transform::Identity;
    SampledEventRange m_sampledEventRange;
};

/// @brief Animation graph node responsible for managing time. Contains the logic needed to calculate the final pose.
/// Tracka and update time per node
/// Synchronization
/// Register pose generation tasks
/// Sampling and modifying root motion deltas
/// Sampling anim events
class PoseNode : public GraphNode
{
  protected:
    class Settings : public GraphNode::Settings
    {
    };

    uint32_t m_loopCount = 0;
    Seconds m_duration = 0.0f;
    Percentage m_currentTime = 0.0f;  // Clamped percentage over the duration
    Percentage m_previousTime = 0.0f; // Clamped percentage over the duration

  private:
    virtual void Initialize(GraphContext& context) override final { Initialize(context, SyncTrackTime()); }
    virtual void InitializeInternal(GraphContext& context) override final { Initialize(context, SyncTrackTime()); }
    virtual NodeValueType GetValueType() const override final { return NodeValueType::Pose; }

  public:
    // Get internal animation state
    virtual const SyncTrack& GetSyncTrack() const = 0;
    inline uint32_t GetLoopCount() const { return m_loopCount; }
    inline const Percentage& GetPreviousTime() const { return m_previousTime; }
    inline const Percentage& GetCurrentTime() const { return m_currentTime; }
    inline Seconds GetDuration() const { return m_duration; }

    // Initialize an animation node with a specific start time
    void Initialize(GraphContext& context, const SyncTrackTime& InitialTime = SyncTrackTime());
    virtual void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime);

    // Unsynchronized update
    virtual PoseNodeResult Update(GraphContext& context) = 0;

    // Synchronized update
    virtual PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) = 0;

    // Deactivate a previous active branch, this is needed when triggering transitions
    virtual void DeactivateBranch(GraphContext& context) { assert(context.m_branchState == BranchState::Inactive && IsNodeActive(context)); }
};
} // namespace aln