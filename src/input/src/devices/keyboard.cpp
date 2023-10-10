#include "devices/keyboard.hpp"

#include "control_state_event.hpp"
#include "input_device.hpp"

namespace aln
{

void Keyboard::UpdateControlState(const Keyboard::Key& keyCode, const ButtonState& buttonState)
{
    auto& keyControl = m_keys[(uint16_t) keyCode];
    keyControl.SetValue(buttonState);

    // Create and populate a control state changed event
    auto& event = m_statesChanged.emplace_back();
    event.m_pControl = &keyControl;
}
} // namespace aln