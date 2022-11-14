#pragma once

#include <map>

namespace aln::input
{
struct ControlStateChangedEvent;

class IInputDevice
{
  protected:
    std::multimap<int, ControlStateChangedEvent> m_statesChanged;

    virtual void Update() = 0;

  public:
    /// @brief Return a list of state changed events that occured since the last call to this method.
    virtual std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() = 0;
};
} // namespace aln::input