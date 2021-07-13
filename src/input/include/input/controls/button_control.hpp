#pragma once
#include "input_control.hpp"

/// @brief Control for arbitrary buttons.
/// TODO: Accept an actuation threshold
class ButtonControl : public InputControl<float>
{
    // TODO: Find a better way to do this
    friend class Mouse;

  public:
    ButtonControl(int code)
    {
        m_defaultValue = 0.;
        m_value = 0.;
        m_id = code;
        // TODO: For now we use GLFW scancodes as ids. We should use a guid
    }
};