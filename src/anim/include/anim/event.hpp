#pragma once

#include "animation.hpp"

#include <cstdint>

namespace aln
{
class AnimationEvent
{
    // Immediate or durable
    enum class Type : uint8_t
    {
        Immediate,
        Durable,
    };

    Type m_type = Type::Immediate;
};

struct SampledEvent
{
    // TODO:
    // Source animation
    Animation* pAnimation;
    AnimationEvent* pEvent;
    float eventWeight = 1; // How important is this event for this update.
    // TODO: Is modified by any blend that occur. When blending away from the source anim, the weight is reduced
    float percent;                   // How long are we through the event (100% for immediate)
    bool isIgnored = false;          // if coming from another layer
    bool fromInactiveBranch = false; // was this event sampled from a graph branch that we are leaving
};
} // namespace aln