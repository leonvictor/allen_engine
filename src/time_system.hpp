#pragma once

#include <chrono>

/// @brief Interface to get time information. Use as a singleton (i.e. Time::GetTime())
/// @todo Unity uses a "fake" timer to enforce a maximum delta time
class Time
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
    static void Update()
    {
        auto& singleton = Singleton();

        auto frameTime = Clock::now();
        singleton.m_deltaTime = frameTime - singleton.m_frameTime;
        singleton.m_frameTime = frameTime;

        singleton.m_frameCount++;
    }

    /// @brief Get the time system singleton.
    static Time& Singleton()
    {
        static Time time;
        return time;
    }

    Time()
    {
        m_startTime = Clock::now();
        m_frameTime = m_startTime;
        m_frameCount = 0;
    }

  public:
    /// @brief Interval since last frame (in seconds).
    static float GetDeltaTime() { return Singleton().m_deltaTime.count(); }

    /// @brief Time at the beginning of this frame (in seconds).
    static float GetTime()
    {
        auto& singleton = Singleton();
        auto diff = singleton.m_frameTime - singleton.m_startTime;
        return diff.count();
    }

    /// @brief Number of frames since the start of the application.
    static uint64_t GetFrameCount() { return Singleton().m_frameCount; }

    /// @brief Real time since the start of the application (in seconds).
    static float GetTimeSinceStartup()
    {
        auto& singleton = Singleton();
        auto diff = Clock::now() - singleton.m_startTime;
        return diff.count();
    }
};