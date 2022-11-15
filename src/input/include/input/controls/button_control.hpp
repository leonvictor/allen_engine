#pragma once

#include "input_control.hpp"

namespace aln::input
{

/// @brief Control for simple buttons.
/// TODO: Update to manage various keyboard layouts
class ButtonControl : public InputControl<ButtonState>
{
    friend class Keyboard;
    friend class Mouse;

  private:
  public:
    ButtonControl()
    {
        m_defaultValue = ButtonState::None;
        m_value = ButtonState::None;
    }

    bool IsHeld() const { return m_value == ButtonState::Held || m_value == ButtonState::Pressed; }
    bool WasPressed() const { return m_value == ButtonState::Pressed; }
    bool WasReleased() const { return m_value == ButtonState::Released; }

    void Update() override
    {
        // Mark the control for update *next frame* if it has changed during this one
        if (m_updateState == UpdateState::Changed)
        {
            m_updateState = UpdateState::RequiresUpdate;
            return;
        }

        if (m_updateState == UpdateState::RequiresUpdate)
        {
            if (m_value == ButtonState::Pressed)
            {
                m_value = ButtonState::Held;
            }
            else if (m_value == ButtonState::Released)
            {
                m_value = ButtonState::None;
            }
        }
    }
};
} // namespace aln::input