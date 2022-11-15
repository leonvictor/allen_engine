#include "mouse.hpp"
#include "control_state_event.hpp"

#include <GLFW/glfw3.h>

namespace aln::input
{

// 1:1 Map from glfw mouse button input codes to ours
const std::unordered_map<uint8_t, Mouse::Button> Mouse::GlfwButtonMap = {
    {GLFW_MOUSE_BUTTON_LEFT, Mouse::Button::Left},
    {GLFW_MOUSE_BUTTON_RIGHT, Mouse::Button::Right},
    {GLFW_MOUSE_BUTTON_MIDDLE, Mouse::Button::Middle},
    {GLFW_MOUSE_BUTTON_4, Mouse::Button::Button4},
    {GLFW_MOUSE_BUTTON_5, Mouse::Button::Button5},
    {GLFW_MOUSE_BUTTON_6, Mouse::Button::Button6},
    {GLFW_MOUSE_BUTTON_7, Mouse::Button::Button7},
    {GLFW_MOUSE_BUTTON_8, Mouse::Button::Button8},
};

/// @brief Return a list of state changed events that occured since the last call to this function.
/// TODO: Share this behavior with Keyboard (and other devices)
/// This probably means moving m_buttons/m_keys to a common m_control
std::multimap<int, ControlStateChangedEvent> Mouse::PollControlChangedEvents()
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
    auto numButtons = m_buttons.size();
    for (auto buttonIdx = 0; buttonIdx < numButtons; ++buttonIdx)
    {
        // Populate the list
        if (!m_statesChanged.contains(buttonIdx) && m_buttons[(uint8_t) buttonIdx].IsActuated())
        {
            auto& event = m_statesChanged.emplace(std::make_pair(buttonIdx, ControlStateChangedEvent()))->second;
            event.pControl = &m_buttons[(uint8_t) buttonIdx];
        }
    }

    std::multimap<int, ControlStateChangedEvent> clone;
    clone.insert(m_statesChanged.begin(), m_statesChanged.end());

    m_statesChanged.clear();
    return clone;
}

void Mouse::UpdateControlState(int code, int action)
{
    // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
    if (action == GLFW_REPEAT)
    {
        return;
    }

    // Find the control if it has already been added to the device, create it otherwise
    auto iter = GlfwButtonMap.find(code);
    assert(iter != GlfwButtonMap.end());

    // Update the control value
    if (action == GLFW_PRESS)
    {
        m_buttons[(uint8_t) iter->second].SetValue(ButtonState::Pressed);
    }

    else if (action == GLFW_RELEASE)
    {
        m_buttons[(uint8_t) iter->second].SetValue(ButtonState::Released);
    }

    // Create and populate a control state changed event
    auto& event = m_statesChanged.emplace(std::make_pair(code, ControlStateChangedEvent()))->second;
    event.pControl = &m_buttons[(uint8_t) iter->second];
}

void Mouse::UpdateScrollControlState(float xdelta, float ydelta)
{
    m_scrollControl.SetValue(ydelta);

    // todo: fix: we need an event when scroll is touched
    // auto& event = m_statesChanged.emplace(std::make_pair(TEMPORARY_SCROLL_ID, ControlStateChangedEvent()))->second;
    // event.pControl = &m_scrollControl;

    m_scrollDelta = glm::vec2(xdelta, ydelta);
}
} // namespace aln::input