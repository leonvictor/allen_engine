#pragma once

#include <common/string_id.hpp>
#include "types.hpp"

#include <cstdint>
#include <assert.h>

namespace aln
{
class AnimationClip;

/// @brief Animation event are associated to a specific time in an animation clip, and are fired when said time is sampled
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
    float m_duration = 0.0f;  // Duration in seconds
};

class SampledEvent
{
  private:
    // TODO: Event ptr is for anim event and id is for state events. They are mutually exclusive. Use an union ?
    AnimationEvent* m_pEvent = nullptr;
    StringID m_stateEventID = StringID::InvalidID;

    NodeIndex m_sourceNodeIndex = InvalidIndex;
    float m_eventWeight = 1.0f; // How important is this event for this update.
    // TODO: Is modified by any blend that occur. When blending away from the source anim, the weight is reduced
    float m_percent = 1.0f; // How long are we through the event (100% for immediate)

    // Metadata
    // TODO: In a "flags" enum
    bool m_isIgnored = false;          // if coming from another layer
    bool m_fromInactiveBranch = false; // was this event sampled from a graph branch that we are leaving

    SampledEvent() = delete;

  public:
    /// @brief Construct a sample state event
    SampledEvent(NodeIndex sourceNodeIndex, const StringID& stateEventID) : m_sourceNodeIndex(sourceNodeIndex), m_stateEventID(stateEventID) {}

    /// @brief Construct a sampled animation event
    SampledEvent (NodeIndex sourceNodeIndex, AnimationEvent* pEvent) { assert(false); } // TODO

    bool IsStateEvent() const { return m_pEvent == nullptr && m_stateEventID != StringID::InvalidID; }
    bool IsAnimationEvent() const { return m_pEvent != nullptr && m_stateEventID == StringID::InvalidID; }

    const StringID& GetStateEventID() const
    {
        assert(IsStateEvent());
        return m_stateEventID;
    }
};
} // namespace aln