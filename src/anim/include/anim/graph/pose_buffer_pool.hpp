#pragma once

#include "../pose.hpp"
#include "../types.hpp"

#include <vector>

namespace aln
{

/// @brief A pose buffer contains a pre-allocated pose which can be passed around and reused between tasks.
/// See the Task class for manipulation routines.
struct PoseBuffer
{
    TaskIndex m_owner = InvalidIndex;
    Pose m_pose;

    bool IsOwned() const { return m_owner != InvalidIndex; }
    // We have a problem here:
    // Pose buffers are kept in a std::vector, which can be dynamically extended. When it does, it can either:
    // * copy its elements around -> we can't do that, as poses shouldn't be copied
    // * create empty elements, and move from the old ones -> meh, but it's what we use here
    // In order to do this we need:
    // * A default constructor which creates a pose from bogus arguments
    // * To delete the copy constructor
    // * A move constructor
    // * A regular constructor
    PoseBuffer() : m_pose(nullptr, Pose::InitialState::None){};
    PoseBuffer(const PoseBuffer& other) = delete; // No copy
    PoseBuffer(PoseBuffer&& other) noexcept = default;
    PoseBuffer(const Skeleton* pSkeleton) : m_pose(pSkeleton, Pose::InitialState::None) {}
};

struct PoseBufferPool
{
    std::vector<PoseBuffer> m_buffers;
    const Skeleton* m_pSkeleton;

    PoseBufferPool(const Skeleton* pSkeleton) : m_pSkeleton(pSkeleton)
    {
        // Average pool size = 5. TODO: is it ?
        constexpr PoseBufferIndex averagePoolSize = 5;
        m_buffers.reserve(averagePoolSize);
        for (PoseBufferIndex bufferIndex = 0; bufferIndex < averagePoolSize; ++bufferIndex)
        {
            m_buffers.emplace_back(m_pSkeleton);
        }
    }

    PoseBufferIndex GetFirstAvailableBufferIndex()
    {
        PoseBufferIndex bufferCount = m_buffers.size();
        for (PoseBufferIndex bufferIndex = 0; bufferIndex < bufferCount; ++bufferIndex)
        {
            if (!m_buffers[bufferIndex].IsOwned())
            {
                return bufferIndex;
            }
        }
        auto& buffer = m_buffers.emplace_back(m_pSkeleton);
        return bufferCount;
    }

    PoseBuffer* GetByIndex(PoseBufferIndex index)
    {
        assert(index < m_buffers.size());
        return &m_buffers[index];
    }

    void ReleasePoseBuffer(PoseBufferIndex index)
    {
        assert(m_buffers[index].IsOwned());
        m_buffers[index].m_owner = InvalidIndex;
    }
};
} // namespace aln