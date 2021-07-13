#include "time_system.hpp"

template class ISingleton<Time>;
void Time::Update()
{
    auto& singleton = Singleton();

    auto frameTime = Clock::now();
    singleton.m_deltaTime = frameTime - singleton.m_frameTime;
    singleton.m_frameTime = frameTime;

    singleton.m_frameCount++;
}

Time::Time()
{
    m_startTime = Clock::now();
    m_frameTime = m_startTime;
    m_frameCount = 0;
}

float Time::GetDeltaTime() { return Singleton().m_deltaTime.count(); }

float Time::GetTime()
{
    auto& singleton = Singleton();
    auto diff = singleton.m_frameTime - singleton.m_startTime;
    return diff.count();
}

uint64_t Time::GetFrameCount() { return Singleton().m_frameCount; }

float Time::GetTimeSinceStartup()
{
    auto& singleton = Singleton();
    auto diff = Clock::now() - singleton.m_startTime;
    return diff.count();
}