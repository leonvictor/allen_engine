#pragma once

#include <chrono>

#include <common/services/service.hpp>

namespace aln
{
/// @brief Interface to get time information
/// @todo use a "fake" timer to enforce a maximum delta time
/// @todo add a way to pause the world
class TimeService : public IService
{
    friend class Engine;

  private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;

    TimePoint m_startTime;
    TimePoint m_frameTime;
    Duration m_deltaTime;

    /// Number of frames since the start of the application
    /// @todo This *will* overflow
    uint64_t m_frameCount;

    /// @brief Notify the time system that we changed frames. Get the current time and update internal fields accordingly.
    void Update();

    TimeService() : m_startTime(Clock::now()), m_frameTime(m_startTime), m_frameCount(0) {};

  public:
    /// @brief Interval since last frame (in seconds).
    float GetDeltaTime() const;

    /// @brief Time at the beginning of this frame (in seconds).
    float GetTime() const;

    /// @brief Number of frames since the start of the application.
    uint64_t GetFrameCount() const;

    /// @brief Real time since the start of the application (in seconds).
    float GetTimeSinceStartup() const;
};
} // namespace aln