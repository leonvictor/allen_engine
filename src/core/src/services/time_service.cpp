#include "services/time_service.hpp"

namespace aln
{

void TimeService::Update()
{
    auto frameTime = Clock::now();
    m_deltaTime = frameTime - m_frameTime;
    m_frameTime = frameTime;

    m_frameCount++;
}


float TimeService::GetDeltaTime() const { return m_deltaTime.count(); }

float TimeService::GetTime() const
{
    auto diff = m_frameTime - m_startTime;
    return diff.count();
}

uint64_t TimeService::GetFrameCount() const { return m_frameCount; }

float TimeService::GetTimeSinceStartup() const
{
    auto diff = Clock::now() - m_startTime;
    return diff.count();
}
} // namespace aln