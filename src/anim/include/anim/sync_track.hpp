#pragma once

#include <aln_anim_export.h>

#include "event.hpp"

#include <common/types.hpp>
#include <common/maths/maths.hpp>

#include <string>
#include <vector>
#include <assert.h>

namespace aln
{
using Percentage = float;

/// @brief Position on a sync track = event index + percentage through this event
struct SyncTrackTime
{
    uint32_t m_eventIdx = InvalidIndex;
    Percentage m_percent = -1.0f;

    float ToFloat() const { return m_percent + m_eventIdx; }
};

struct SyncTrackTimeRange
{
    SyncTrackTime m_beginTime;
    SyncTrackTime m_endTime;
};

class ALN_ANIM_EXPORT SyncTrack
{
    struct Event
    {
        // TODO: StringID
        std::string m_name;
        // In percentage
        float m_startTime = 0.0f;
        float m_duration = 1.0f;
    };

    std::vector<SyncTrack::Event> m_events;

  public:
    /// @brief Default construction creates a single sync event over the whole track
    SyncTrack()
    {
        m_events.emplace_back();
    }

    /// @brief Create a new sync track by blending two existing ones
    static SyncTrack Blend(const SyncTrack& source, const SyncTrack& target, const float blendWeight)
    {
        assert(blendWeight >= 0.0f && blendWeight <= 1.0f);

        // TODO: Temporary assert. Handle different track sizes
        assert(source.m_events.size() == target.m_events.size());

        SyncTrack blendedTrack;
        blendedTrack.m_events.clear(); // TODO: dirty dirty
        const auto eventCount = target.m_events.size();
        for (auto eventIdx = 0; eventIdx < eventCount; ++eventIdx)
        {
            const auto& sourceEvent = source.m_events[eventIdx];
            const auto& targetEvent = target.m_events[eventIdx];

            // TODO: For now we only support "fake" sync tracks which represent full clips
            assert(sourceEvent.m_duration == 1.0f && targetEvent.m_duration == 1.0f);
            assert(sourceEvent.m_startTime == 0.0f && targetEvent.m_startTime == 0.0f);

            // TODO: Scale the input tracks

            auto& event = blendedTrack.m_events.emplace_back();
            event.m_duration = Maths::Lerp(sourceEvent.m_duration, targetEvent.m_duration, blendWeight);
            event.m_startTime = 0.0f; // TODO
            event.m_name = blendWeight <= 0.5 ? sourceEvent.m_name : targetEvent.m_name;
        }

        return blendedTrack;
    }

    size_t GetEventCount() const { return m_events.size(); }
    
    /// @brief Returns the sync time at the specified progress through the track
    /// @todo Handle looping
    SyncTrackTime GetTime(float progressPercent) const {
        assert(progressPercent >= 0.0f && progressPercent <= 1.0f);
        
        SyncTrackTime time;
        
        const auto eventCount = m_events.size();
        for (auto eventIdx = 0; eventIdx < eventCount; ++eventIdx)
        {
            const auto& event = m_events[eventIdx];
            if (event.m_startTime + event.m_duration > progressPercent)
            {
                time.m_eventIdx = eventIdx;
                time.m_percent = (progressPercent - event.m_startTime) / event.m_duration;
                break;
            }
        }
        
        assert(time.m_eventIdx != InvalidIndex);
        return time;
    }

    float GetPercentageThrough(const SyncTrackTime& time) const { 
        assert(time.m_eventIdx < m_events.size());
        assert(time.m_percent >= 0.0f && time.m_percent <= 1.0f);
        
        const auto& event = m_events[time.m_eventIdx];
        return event.m_startTime + (event.m_duration * time.m_percent);
    }
    
    static float CalculateSynchronizedTrackDuration(float sourceDuration, float targetDuration, const SyncTrack& sourceTrack, const SyncTrack& targetTrack, const SyncTrack& blendedTrack, const float blendWeight)
    {
        const float scaledSourceDuration = sourceDuration * ((float) blendedTrack.GetEventCount() / (float) sourceTrack.GetEventCount());
        const float scaledTargetDuration = targetDuration * ((float) blendedTrack.GetEventCount() / (float) targetTrack.GetEventCount());
        return Maths::Lerp(scaledSourceDuration, scaledTargetDuration, blendWeight);
    }

    static const SyncTrack Default;
};
} // namespace aln