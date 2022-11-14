#include "keyboard.hpp"

#include "control_state_event.hpp"
#include "input_device.hpp"

#include <GLFW/glfw3.h>

#include <glm/vec2.hpp>
#include <map>

namespace aln::input
{

std::multimap<int, ControlStateChangedEvent> Keyboard::PollControlChangedEvents()
{
    // TODO: How do I handle multiple changes per frame ?
    // Scenarios:
    // 1. Loop over all keys when polling to get the actuated ones
    //      -> We're missing just released ones
    // 2. Update the event list when receiving glfw events and complete it when polling
    //      -> Ok, but only with a simple map, so a single change per frame
    // 3. Same as 2. but only update the ones that are not yet in the event list
    //      -> Yeah sure. Can we do this without unnecesary loops ?

    // This is 3.
    /// FIXME: Too many loops
    for (auto iter = m_keys.begin(); iter != m_keys.end(); ++iter)
    {
        // Populate the list
        if (!m_statesChanged.contains(iter->first) && iter->second.IsActuated())
        {
            auto& event = m_statesChanged.emplace(std::make_pair(iter->first, ControlStateChangedEvent()))->second;
            event.pControl = &iter->second;
        }
    }

    // TODO: Not sure that works
    std::multimap<int, ControlStateChangedEvent> clone;
    clone.merge(m_statesChanged);

    m_statesChanged.clear();
    return clone;
}

void Keyboard::UpdateControlState(int code, int action)
{
    // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
    if (action == GLFW_REPEAT)
    {
        return;
    }

    // TODO: Add buttons statically once at app launch and use map.find here
    // Find the control if it has already been added to the device, create it otherwise
    auto iter = m_keys.emplace(std::make_pair(code, ButtonControl(code))).first;

    // Update the control value
    if (action == GLFW_PRESS)
    {
        iter->second.SetValue(ButtonState::Pressed);
    }

    else if (action == GLFW_RELEASE)
    {
        iter->second.SetValue(ButtonState::Released);
    }

    // Create and populate a control state changed event
    auto& event = m_statesChanged.emplace(std::make_pair(code, ControlStateChangedEvent()))->second;
    event.pControl = &iter->second;
}

const ButtonControl& Keyboard::GetKey(int code) const
{
    auto iter = m_keys.find(code);
    if (iter != m_keys.end())
    {
        return iter->second;
    }

    throw;
}
} // namespace aln::input