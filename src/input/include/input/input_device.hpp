#pragma once

#include "control_state_event.hpp"

#include <common/containers/hash_map.hpp>
#include <common/containers/vector.hpp>
#include <common/uuid.hpp>

namespace aln
{

class IInputDevice
{
  protected:
    /// @brief Controls that changed since the last frame
    Vector<ControlStateChangedEvent> m_statesChanged;

    virtual void Update() = 0;
    virtual void ClearFrameState() {}

  public:
    /// @brief Poll the list of state changed events that occured since the last call to this method, and clear the cached ones.
    /// TODO: Experiment with other data structures
    virtual void PollControlChangedEvents(HashMap<UUID, ControlStateChangedEvent>& out)
    {
        for (auto& event : m_statesChanged)
        {
            out[event.m_pControl->GetID()] = event;
        }

        m_statesChanged.clear();
    }
};

} // namespace aln