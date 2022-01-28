#pragma once

#include "../pose.hpp"
#include "types.hpp"

#include <vector>

namespace aln
{

// TODO
struct PoseBuffer
{
    TaskIndex m_owner = InvalidIndex;
    Pose m_pose;
};

struct PoseBufferPool
{
    std::vector<PoseBuffer> m_buffers;

    PoseBufferPool()
    {
        m_buffers.resize(5);
    }

    std::pair<PoseBufferIndex, PoseBuffer*> GetFirstAvailable()
    {
        auto size = m_buffers.size();
        for (uint8_t i = 0; i < size; ++i)
        {
            if (m_buffers[i].m_owner == InvalidIndex)
            {
                return {i, &m_buffers[i]};
            }
        }
        auto& buffer = m_buffers.emplace_back();
        return {size, &buffer};
    }

    PoseBuffer* GetByIndex(PoseBufferIndex index)
    {
        assert(index < m_buffers.size());
        return &m_buffers[index];
    }
};
} // namespace aln