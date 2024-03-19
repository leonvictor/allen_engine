#pragma once

#include <common/types.hpp>

#include <cstdint>

namespace aln
{
/// @brief Represent a time point in a animation through a frame idx and a percentage through that frame
class FrameTime
{
  private:
    uint32_t m_frameIndex = 0;
    Percentage m_percentageThroughFrame = 0.0f;

  public:
    FrameTime(uint32_t frameIndex, Percentage percentageThroughFrame) : m_frameIndex(frameIndex), m_percentageThroughFrame(percentageThroughFrame) {}
    
    uint32_t GetFrameIndex() const { return m_frameIndex; }
    Percentage GetPercentageThroughFrame() const { return m_percentageThroughFrame; }
    bool IsOnKeyFrame() const { return m_percentageThroughFrame == 0.0f || m_percentageThroughFrame == 1.0f; }
    
    explicit operator float() const { return m_percentageThroughFrame + m_frameIndex; }
};
}