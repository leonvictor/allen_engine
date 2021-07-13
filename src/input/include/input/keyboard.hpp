#pragma once

#include "control_state_event.hpp"
#include "controls/key_control.hpp"
#include "input_device.hpp"

#include <glm/vec2.hpp>
#include <map>

namespace aln
{
class Engine;
namespace input
{

/// @brief Describes a physical keyboard.
class Keyboard : public IInputDevice
{
    friend aln::Engine;
    friend class Input; // TODO: Add an indirection level to let input handle this

  private:
    /// Map of keyboard keys controls.
    /// @note We use a map as sets elements cannot be modified in place (see https://en.cppreference.com/w/cpp/container/unordered_set)
    std::map<int, KeyControl> m_keys;

    std::multimap<int, ControlStateChangedEvent> m_statesChanged;

    /// @brief Return a list of state changed events that occured since the last call to this function.
    std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() override
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
        std::multimap<int, ControlStateChangedEvent> clone = m_statesChanged;
        m_statesChanged.clear();
        return clone;
    }

    /// @brief Translate a GLFW Event to KeyControl
    /// @todo Move to virtual fn in InputDevice (possibly InputControl even ?)
    /// @todo Shouldn't be public
    void UpdateControlState(int code, int action)
    {
        // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
        if (action == GLFW_REPEAT)
        {
            return;
        }

        // Find the control if it has already been added to the device, create it otherwise
        auto iter = m_keys.emplace(std::make_pair(code, KeyControl(code))).first;
        // Update the control value
        iter->second.SetValue(MapGLFWActionCode(action));
        // SetControlValue(iter->second, MapGLFWActionCode(action));

        // Create and populate a control state changed event
        auto& event = m_statesChanged.emplace(std::make_pair(code, ControlStateChangedEvent()))->second;
        event.pControl = &iter->second;
    }

  public:
    KeyControl& GetKey(int code)
    {
        auto iter = m_keys.find(code);
        if (iter != m_keys.end())
        {
            return iter->second;
        }

        throw;
    }
};
} // namespace input
} // namespace aln