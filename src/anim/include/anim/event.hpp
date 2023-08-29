#pragma once

#include <cstdint>

namespace aln
{
class AnimationClip;

class AnimationEvent
{
    // Immediate or durable
    enum class Type : uint8_t
    {
        Immediate,
        Durable,
    };

    Type m_type = Type::Immediate;
    float m_startTime = 0.0f; // Start time in seconds
    float m_duration = 0.0f; // Duration in seconds
};

struct SampledEvent
{
    // TODO:
    // Source animation
    AnimationClip* m_pSourceAnimation = nullptr;
    AnimationEvent* m_pEvent = nullptr;
    float m_eventWeight = 1.0f; // How important is this event for this update.
    // TODO: Is modified by any blend that occur. When blending away from the source anim, the weight is reduced
    float m_percent = 1.0f;                   // How long are we through the event (100% for immediate)
    
    // Metadata
    // TODO: In an enum
    bool m_isIgnored = false;          // if coming from another layer
    bool m_fromInactiveBranch = false; // was this event sampled from a graph branch that we are leaving
};
} // namespace aln