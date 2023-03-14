#pragma once

#include "event.hpp"

namespace aln
{
using Percentage = float;

struct SyncTrackTime
{
    AnimationEvent event;
    Percentage percent;
};

struct SyncTrackTimeRange
{
    SyncTrackTime m_beginTime;
    SyncTrackTime m_endTime;
};
class SyncTrack
{
};
} // namespace aln