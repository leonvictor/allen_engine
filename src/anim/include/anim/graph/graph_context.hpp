#pragma once

#include <vector>

#include "../skeleton.hpp"

#include <common/transform.hpp>

namespace aln
{

// TODO: Where ?
enum class BranchState
{
    Inactive,
    Active,
};

// TODO
struct PoseBuffer
{
    Pose m_pose;
};

// TODO
struct TaskSystem
{
    std::vector<PoseBuffer> m_poseBufferPool;
    // TODO: Tasks will request, free and transfer pose buffers from the pool
    // Pool dynamically grow if more poses are requested
    // avg size = ~5 poses per pool
};

struct SampledEventsBuffer
{
};

struct BoneMasksPool
{
};

struct GraphLayerContext
{
};
struct RootMotionActionRecorder
{
};

using NodeIndex = uint32_t;
using Seconds = float;

class GraphContext
{
  public:
    static const NodeIndex InvalidIndex = UINT32_MAX;

    const TaskSystem* m_pTaskSystem = nullptr;
    const Skeleton const* m_pSkeleton = nullptr;
    const Pose const* m_pPreviousPose = nullptr;
    Seconds m_deltaTime = 0.0f;
    Transform m_worldTransform = Transform::Identity;
    Transform m_worldTransformInverse = Transform::Identity;
    SampledEventsBuffer m_sampledEvents;
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
    GraphContext();

    void Initialize(TaskSystem* pTaskSystem, const Pose* pPreviousPose);
    void Shutdown();

    inline bool IsValid() const { return m_pSkeleton != nullptr && m_pTaskSystem != nullptr && m_pPreviousPose != nullptr; }
    void Update(const Seconds deltaTime, const Transform& currentWorldTransform);

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