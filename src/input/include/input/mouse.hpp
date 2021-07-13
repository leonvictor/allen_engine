#pragma once

#include "controls/axis_control.hpp"
#include "controls/button_control.hpp"
#include "input_device.hpp"

#include <glfw/glfw3.h>
#include <glm/vec2.hpp>
#include <vector>

class Engine;
class Input;

namespace input::devices
{
/// @brief Describe a physical mouse and its on-screen cursor relative.
class Mouse : IInputDevice
{
    friend class Engine;
    friend class Input;

  private:
    // Position in screen space.
    glm::vec2 m_position;

    // Difference in position since last frame.
    glm::vec2 m_delta;

    std::map<int, ButtonControl> m_buttons;

    // Difference in position of the scroller since last frame.
    glm::vec2 m_scrollDelta;

    AxisControl m_scrollControl;

    /// @brief Update position and delta according to the new provided position.
    void Update(glm::vec2 position)
    {
        m_delta = position - m_position;
        m_position = position;
    }

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

        std::multimap<int, ControlStateChangedEvent> clone = m_statesChanged;
        m_statesChanged.clear();
        return clone;
    }

    /// @brief Translate a GLFW Event to KeyControl
    // TODO: Move to virtual fn in InputDevice (possibly InputControl even ?)
    void UpdateControlState(int code, int action)
    {
        // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
        if (action == GLFW_REPEAT)
        {
            return;
        }

        // Find the control if it has already been added to the device, create it otherwise
        auto iter = m_buttons.emplace(std::make_pair(code, ButtonControl(code))).first;
        // Update the control value
        iter->second.SetValue((float) MapGLFWActionCode(action));

        // Create and populate a control state changed event
        auto& event = m_statesChanged.emplace(std::make_pair(code, ControlStateChangedEvent()))->second;
        event.pControl = &iter->second;
    }

    void UpdateScrollControlState(float xdelta, float ydelta)
    {
        m_scrollControl.SetValue(ydelta);
        auto& event = m_statesChanged.emplace(std::make_pair(TEMPORARY_SCROLL_ID, ControlStateChangedEvent()))->second;
        event.pControl = &m_scrollControl;

        m_scrollDelta = glm::vec2(xdelta, ydelta);
    }

  public:
    const int TEMPORARY_SCROLL_ID = 11111;

    /// @brief Default constructor creates standard mouse controls (right and left click, scrolling wheel).
    Mouse()
    {
        m_buttons.emplace(std::make_pair(GLFW_MOUSE_BUTTON_1, ButtonControl(GLFW_MOUSE_BUTTON_1)));
        m_buttons.emplace(std::make_pair(GLFW_MOUSE_BUTTON_2, ButtonControl(GLFW_MOUSE_BUTTON_2)));
        m_buttons.emplace(std::make_pair(GLFW_MOUSE_BUTTON_3, ButtonControl(GLFW_MOUSE_BUTTON_3)));

        // FIXME: ID should be unique
        m_scrollControl = AxisControl(TEMPORARY_SCROLL_ID);
    }

    const glm::vec2& GetPosition() const { return m_position; }
    const glm::vec2& GetDelta() const { return m_delta; }
    const glm::vec2& GetScroll() const { return m_scrollDelta; }
};
} // namespace input::devices