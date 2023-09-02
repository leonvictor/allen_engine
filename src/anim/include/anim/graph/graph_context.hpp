#pragma once

#include <vector>

#include "../pose.hpp"
#include "../skeleton.hpp"
#include "task_system.hpp"

#include <common/transform.hpp>

namespace aln
{

// TODO: Where ?
enum class BranchState : uint8_t
{
    Inactive,
    Active,
};

/// @brief Holds all events sampled during a frame
struct SampledEventsBuffer
{
    using Iterator = std::vector<SampledEvent>::iterator;
    
    std::vector<SampledEvent> m_events;

    SampledEvent& EmplaceStateEvent(NodeIndex sourceNodeIdx, const StringID& eventID)
    {
        return m_events.emplace_back(sourceNodeIdx, eventID);
    }

    void Clear() {        m_events.clear();}
    bool Empty() const { return m_events.empty(); }
    const std::vector<SampledEvent>& GetSampledEvents() const { return m_events; }

    Iterator begin() { return m_events.begin(); }
    Iterator end() { return m_events.end(); }

};

/// @brief The events sampled by a node during a frame, represented by the indices of said events in the buffer 
struct SampledEventRange
{
    uint32_t m_startEventIdx = InvalidIndex;
    uint32_t m_endEventIdx = InvalidIndex;
};

struct BoneMasksPool
{
    // TODO
};

struct GraphLayerContext
{
    // TODO
};

struct RootMotionActionRecorder
{
    // TODO
};

/// @brief Contains all the necessary data for a graph instance to execute
/// Gets passed trough and used by each node.
class GraphContext
{
  public:
    TaskSystem* m_pTaskSystem = nullptr; // Task system to register to
    const Skeleton* m_pSkeleton = nullptr;
    const Pose* m_pPreviousPose = nullptr; // Pose from last frame
    Seconds m_deltaTime = 0.0f;
    Transform m_worldTransform = Transform::Identity;        // World transform of the considered character this frame
    Transform m_worldTransformInverse = Transform::Identity; // Inverse world transform of the character this frame
    SampledEventsBuffer m_sampledEventsBuffer;                     // Event buffer to fill with sampled events
    uint32_t m_updateId = 0;
    BranchState m_branchState = BranchState::Active;

    BoneMasksPool m_boneMasksPool;
    GraphLayerContext m_layerContext;

  private:
#ifndef NDEBUG
    RootMotionActionRecorder m_rootMotionActionRecorder; // Allows nodes to record root motion ops
    std::vector<NodeIndex> m_activeNodes;
#endif

  public:
    GraphContext()
    {
        // TODO
    }

    void Initialize(TaskSystem* pTaskSystem, const Pose* pPreviousPose)
    {
        m_pTaskSystem = pTaskSystem;
        m_pPreviousPose = pPreviousPose;
        m_pSkeleton = pPreviousPose->GetSkeleton();

        // TODO: Initialize Root motion recorder

        m_deltaTime = 0.0f;
        m_worldTransform = Transform::Identity;
        m_worldTransformInverse = Transform::Identity;
        m_branchState = BranchState::Active;
        m_sampledEventsBuffer.Clear();
    }

    void Shutdown()
    {
        m_pSkeleton = nullptr;
        m_pPreviousPose = nullptr;
        m_pTaskSystem = nullptr;
        m_sampledEventsBuffer.Clear();
        // TODO
    }

    inline bool IsValid() const { return m_pSkeleton != nullptr && m_pTaskSystem != nullptr && m_pPreviousPose != nullptr; }

    void Update(const Seconds deltaTime, const Transform& currentWorldTransform)
    {
        // TODO
        m_deltaTime = deltaTime;
        m_worldTransform = currentWorldTransform;
        m_worldTransformInverse = currentWorldTransform.GetInverse();
        m_sampledEventsBuffer.Clear();
    }

// Debugging
#ifndef NDEBUG
    inline void TrackActiveNode(NodeIndex nodeIdx)
    {
        assert(nodeIdx != InvalidIndex);
        m_activeNodes.emplace_back(nodeIdx);
    }
    inline const std::vector<NodeIndex>& GetActiveNodes() const { return m_activeNodes; }
    inline RootMotionActionRecorder* GetRootMotionActionRecorder() { return &m_rootMotionActionRecorder; }
#endif

  private:
    GraphContext(const GraphContext&) = delete;
    GraphContext& operator=(GraphContext&) = delete;
};
} // namespace aln