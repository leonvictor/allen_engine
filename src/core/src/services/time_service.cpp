#include "services/time_service.hpp"

namespace aln
{

void TimeService::Update()
{
    auto frameTime = std::chrono::time_point_cast<Duration>(Clock::now());
    m_deltaTime = frameTime - m_frameTime;
    m_frameTime = frameTime;

    m_frameCount++;
}

float TimeService::GetDeltaTime() const { return m_deltaTime.count(); }

float TimeService::GetTime() const { return m_frameTime.time_since_epoch().count(); }

uint64_t TimeService::GetFrameCount() const { return m_frameCount; }

float TimeService::GetTimeSinceAppStart() const { return m_startTime.time_since_epoch().count(); }
} // namespace aln