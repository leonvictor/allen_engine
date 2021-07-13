#pragma once

#include "input_control.hpp"

#include <glfw/glfw3.h>

namespace aln::input
{
/// @brief Control for keyboard keys.
/// TODO: Update to manage various keyboard layouts
class KeyControl : public InputControl<uint8_t>
{
    friend class Keyboard;

  private:
    int m_code;

  public:
    KeyControl(int code)
    {
        m_defaultValue = 0;
        m_value = 0;
        m_code = code;
        m_id = code; // TODO: For now we use GLFW scancodes as ids
    }
};
}