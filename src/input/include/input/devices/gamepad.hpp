#pragma once

#include "input_device.hpp"

#include "controls/2d_axis_control.hpp"

namespace aln
{
class Gamepad : public IInputDevice
{
    friend class GLFWApplication;

  public:
    enum class Button : uint8_t
    {
        A, // South
        B, // East
        X, // West
        Y, // North
        LeftBumper,
        RightBumper,
        Back, // Select
        Start,
        Guide, // Home, XBOX
        LeftStick,
        RightStick,
        DPADUp,
        DPADRight,
        DPADDown,
        DPADLeft,
    };

  private:
    std::array<ButtonControl, 15> m_buttons;

    AxisControl2D m_rightStick;
    AxisControl2D m_leftStick;
    AxisControl m_rightTrigger;
    AxisControl m_leftTrigger;

    // TODO:
    // - Handle D-Pads
    // - Handle shoulder buttons
    // - Handle start/select

    void SetRightStickState(const glm::vec2& value) { m_rightStick.SetValue(value); }
    void SetLeftStickState(const glm::vec2& value) { m_leftStick.SetValue(value); }
    void SetRightTriggerState(float value) { m_rightTrigger.SetValue(value); }
    void SetLeftTriggerState(float value) { m_leftTrigger.SetValue(value); }

    void SetButtonPressed(Button button) { m_buttons[(uint8_t) button].Press(); }
    void SetButtonReleased(Button button) { m_buttons[(uint8_t) button].Release(); }

  public:
    void Update() override
    {
        for (auto& button : m_buttons)
        {
            button.Update();
        }
    }

    std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() override
    {
        // TODO: Rework input callbacks. Maybe even get rid of them ? Or make sure callback do not happen anytime
        return std::multimap<int, ControlStateChangedEvent>();
    }

    inline const glm::vec2& GetLeftStickValue() const { return m_leftStick.GetValue(); } 
    inline const glm::vec2& GetRightStickValue() const { return m_rightStick.GetValue(); } 
    inline float GetLeftTriggerValue() const { return m_leftTrigger.GetValue(); }
    inline float GetRightTriggerValue() const { return m_rightTrigger.GetValue(); } 
    inline bool WasPressed(Button button) const { return m_buttons[(uint8_t) button].WasPressed(); }
    inline bool WasReleased(Button button) const { return m_buttons[(uint8_t) button].WasReleased(); }
    inline bool IsHeld(Button button) const { return m_buttons[(uint8_t) button].IsHeld(); }
};
} // namespace aln