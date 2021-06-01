#pragma once
#include "input_control.hpp"

/// @brief Floating point axis control.
class AxisControl : public InputControl<float>
{
    // TODO: Find a better way to do this
    friend class Mouse;

  public:
    AxisControl(){};
    AxisControl(int code)
    {
        m_defaultValue = 0.;
        m_value = 0.;
        m_id = code;
        // TODO: For now we use GLFW scancodes as ids. We should use a guid
    }
};