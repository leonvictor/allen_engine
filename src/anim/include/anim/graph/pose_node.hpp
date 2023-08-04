    #pragma once

#include <assert.h>
#include <vector>

#include <common/transform.hpp>

#include "../event.hpp"
#include "../sync_track.hpp"
#include "../types.hpp"
#include "graph_context.hpp"
#include "runtime_graph_node.hpp"

namespace aln
{

struct SampledEventRange
{
};

struct PoseNodeResult
{
    TaskIndex m_taskIndex = InvalidIndex;              // Registered task index
    Transform m_rootMotionDelta = Transform::Identity; // Delta that was sampled at that node
    SampledEventRange m_sampledEventRange;             // Event range sampled by that node

    inline bool HasRegisteredTasks() const { return m_taskIndex != InvalidIndex; }
};

/// @brief Animation graph node responsible for managing time. Contains the logic needed to calculate the final pose.
class PoseRuntimeNode : public RuntimeGraphNode
{
  protected:
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
    void Initialize(GraphContext& context, const SyncTrackTime& initialTime = SyncTrackTime())
    {
        InitializeInternal(context, initialTime);
    }

    /// @brief Custom init function to be overriden in derived node classes. Custom overrides should initialize:
    /// - Time info (duration, current time, previous time);
    /// - Child nodes
    virtual void InitializeInternal(GraphContext& context, const SyncTrackTime& initialTime)
    {
        RuntimeGraphNode::InitializeInternal(context);

        m_loopCount = 0;
        m_duration = 0.0f;
        m_currentTime = 0.0f;
        m_previousTime = 0.0f;
    }

    /// @brief Unsynchronized update
    /// @note Use the time delta for the current step
    virtual PoseNodeResult Update(GraphContext& context) = 0;

    /// @brief Synchronized update
    /// @note Use the time range given as argument
    virtual PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) = 0;

    // Deactivate a previous active branch, this is needed when triggering transitions
    virtual void DeactivateBranch(GraphContext& context) { assert(context.m_branchState == BranchState::Inactive && IsNodeActive(context)); }
};
} // namespace aln