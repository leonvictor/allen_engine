#pragma once

#include "input_device.hpp"

#include "controls/2d_axis_control.hpp"

namespace aln
{
class Gamepad : public IInputDevice
{
  public:
    enum class Button : uint8_t
    {
        A,
        B,
        X,
        Y,
    };

  private:
    std::map<Button, ButtonControl> m_buttons;

    AxisControl2D m_rightStick;
    AxisControl2D m_leftStick;

    AxisControl m_rightTrigger;
    AxisControl m_leftTrigger;

    // TODO: 
    // - Handle D-Pads
    // - Handle shoulder buttons
    // - Handle start/select

  public:
    void Update() override
    {
        // TODO
    }

    std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() override
    {
        // TODO
    }
};
} // namespace aln