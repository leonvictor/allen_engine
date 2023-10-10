#include "devices/mouse.hpp"

#include "control_state_event.hpp"

#include <GLFW/glfw3.h>

namespace aln
{

void Mouse::UpdateControlState(const Button& button, const ButtonState& buttonState)
{
    auto& buttonControl = m_buttons[(uint8_t) button];
    buttonControl.SetValue(buttonState);

    // Create and populate a control state changed event
    auto& event = m_statesChanged.emplace_back();
    event.m_pControl = &buttonControl;
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