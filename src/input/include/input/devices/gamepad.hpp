#pragma once

#include "input_device.hpp"

#include "controls/2d_axis_control.hpp"

#include <glm/gtx/norm.hpp>

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

    struct AnalogStickDeadzone
    {
        float m_innerRange = 0.0f;
        float m_outerRange = 0.0f;
    };

  private:
    AxisControl2D m_rightStick;
    glm::vec2 m_rightStickFilteredValue = {0.0f, 0.0f};
    AnalogStickDeadzone m_rightStickDeadzone = {0.25, 0.1f};
    
    AxisControl2D m_leftStick;
    glm::vec2 m_leftStickFilteredValue = {0.0f, 0.0f};
    AnalogStickDeadzone m_leftStickDeadzone = {0.25, 0.1f};
    
    AxisControl m_rightTrigger;
    AxisControl m_leftTrigger;

    std::array<ButtonControl, 15> m_buttons;
    
    
    void SetRightStickState(const glm::vec2& value) { 
        m_rightStick.SetValue(value); 
        m_rightStickFilteredValue = ApplyDeadzone(value, m_rightStickDeadzone);
    }

    void SetLeftStickState(const glm::vec2& value)
    {
        m_leftStick.SetValue(value);
        m_leftStickFilteredValue = ApplyDeadzone(value, m_leftStickDeadzone);
    }
    
    void SetRightTriggerState(float value) { m_rightTrigger.SetValue(value); }
    void SetLeftTriggerState(float value) { m_leftTrigger.SetValue(value); }

    void SetButtonPressed(Button button) { m_buttons[(uint8_t) button].Press(); }
    void SetButtonReleased(Button button) { m_buttons[(uint8_t) button].Release(); }

    glm::vec2 ApplyDeadzone(const glm::vec2& rawValue, const AnalogStickDeadzone& deadzone)
    {
        const auto magnitude = glm::length2(rawValue);
        const auto direction = rawValue / magnitude;
        if (magnitude > deadzone.m_innerRange)
        {
            float const remainingRange = (1.0f - deadzone.m_innerRange - deadzone.m_outerRange);
            float remaingMagnitude = glm::min(1.0f, (magnitude - deadzone.m_innerRange) / remainingRange);
            return direction * remaingMagnitude;
        }
        else
        {
            return {0.0f, 0.0f};
        }
    }

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
    
    inline void SetRightStickDeadzone(const AnalogStickDeadzone& deadzone) { m_rightStickDeadzone = deadzone; }
    inline void SetLeftStickDeadzone(const AnalogStickDeadzone& deadzone) { m_leftStickDeadzone = deadzone; }
    
    // --- Analog sticks
    inline const glm::vec2& GetRawLeftStickValue() const { return m_leftStick.GetValue(); } 
    inline const glm::vec2& GetRawRightStickValue() const { return m_rightStick.GetValue(); } 
    
    inline const glm::vec2& GetLeftStickValue() const { return m_leftStickFilteredValue; }
    inline const glm::vec2& GetRightStickValue() const { return m_rightStickFilteredValue; } 
    
    // --- Triggers
    inline float GetLeftTriggerValue() const { return m_leftTrigger.GetValue(); }
    inline float GetRightTriggerValue() const { return m_rightTrigger.GetValue(); } 
    
    // --- Buttons
    inline bool WasPressed(Button button) const { return m_buttons[(uint8_t) button].WasPressed(); }
    inline bool WasReleased(Button button) const { return m_buttons[(uint8_t) button].WasReleased(); }
    inline bool IsHeld(Button button) const { return m_buttons[(uint8_t) button].IsHeld(); }
};
} // namespace aln