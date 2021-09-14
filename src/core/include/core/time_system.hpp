#pragma once

#include <chrono>
#include <utils/singleton.hpp>

namespace aln
{
/// @brief Interface to get time information. Use as a singleton (i.e. Time::GetTime())
/// @todo Unity uses a "fake" timer to enforce a maximum delta time
class Time
{
    friend class ISingleton<Time>;
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
    static void Update();

    static Time& Singleton();

    Time();

  public:
    /// @brief Interval since last frame (in seconds).
    static float GetDeltaTime();

    /// @brief Time at the beginning of this frame (in seconds).
    static float GetTime();

    /// @brief Number of frames since the start of the application.
    static uint64_t GetFrameCount();

    /// @brief Real time since the start of the application (in seconds).
    static float GetTimeSinceStartup();
};
} // namespace aln