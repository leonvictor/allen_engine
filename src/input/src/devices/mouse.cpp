#include "devices/mouse.hpp"

#include "control_state_event.hpp"

#include <GLFW/glfw3.h>

namespace aln
{

// 1:1 Map from glfw mouse button input codes to ours
const HashMap<uint8_t, Mouse::Button> Mouse::GlfwButtonMap = {
    {GLFW_MOUSE_BUTTON_LEFT, Mouse::Button::Left},
    {GLFW_MOUSE_BUTTON_RIGHT, Mouse::Button::Right},
    {GLFW_MOUSE_BUTTON_MIDDLE, Mouse::Button::Middle},
    {GLFW_MOUSE_BUTTON_4, Mouse::Button::Button4},
    {GLFW_MOUSE_BUTTON_5, Mouse::Button::Button5},
    {GLFW_MOUSE_BUTTON_6, Mouse::Button::Button6},
    {GLFW_MOUSE_BUTTON_7, Mouse::Button::Button7},
    {GLFW_MOUSE_BUTTON_8, Mouse::Button::Button8},
};

void Mouse::UpdateControlState(int code, int action)
{
    // TODO: Move mapping to glfw mapping wrapper
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
    auto& event = m_statesChanged.emplace_back();
    event.m_pControl = &m_buttons[(uint8_t) iter->second];
}

void Mouse::UpdateScrollControlState(float xdelta, float ydelta)
{
    m_scrollControl.SetValue(ydelta);

    // todo: fix: we need an event when scroll is touched
    // auto& event = m_statesChanged.emplace(std::make_pair(TEMPORARY_SCROLL_ID, ControlStateChangedEvent()))->second;
    // event.pControl = &m_scrollControl;

    m_scrollDelta = Vec2(xdelta, ydelta);
}
} // namespace aln