#pragma once

#include "controls/button_control.hpp"
#include "input_device.hpp"

#include <glfw/glfw3.h>
#include <glm/vec2.hpp>
#include <vector>

/// @brief Describe a physical mouse and its on-screen cursor relative.
class Mouse : IInputDevice
{
    friend class Engine;

  private:
    // Position in screen space.
    glm::vec2 m_position;

    // Difference in position since last frame.
    glm::vec2 m_delta;

    std::map<int, ButtonControl> m_buttons;

    // Difference in position of the scroller since last frame.
    glm::vec2 m_scrollDelta;

    /// @brief Update position and delta according to the new provided position.
    void Update(glm::vec2 position)
    {
        m_delta = position - m_position;
        m_position = position;
    }

  public:
    /// @brief Default constructor creates "traditionnal" mouse buttons (right, left and scroll clicks)
    Mouse()
    {
    }

    glm::vec2 GetPosition() { return m_position; }
    glm::vec2 GetDelta() { return m_delta; }
    glm::vec2 GetScroll() { return m_scrollDelta; }

    /// @brief Return a list of state changed events that occured since the last call to this function.
    /// TODO: Share this behavior with Keyboard (and other devices)
    /// This probably means moving m_buttons/m_keys to a common m_control
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
        for (auto iter = m_buttons.begin(); iter != m_buttons.end(); ++iter)
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
    // TODO: Move to virtual fn in InputDevice (possibly InputControl even ?)
    void UpdateControlState(int code, int action)
    {
        // UpdateControlState<ButtonControl>(code, action);
        // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
        if (action == GLFW_REPEAT)
        {
            return;
        }

        // Find the control if it has already been added to the device, create it otherwise
        auto iter = m_buttons.emplace(std::make_pair(code, ButtonControl(code))).first;
        // Update the control value
        iter->second.SetValue((float) MapGLFWActionCode(action));
        // SetControlValue(iter->second, (float) MapGLFWActionCode(action));

        // Create and populate a control state changed event
        auto& event = m_statesChanged.emplace(std::make_pair(code, ControlStateChangedEvent()))->second;
        event.pControl = &iter->second;
    }
};