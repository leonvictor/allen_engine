#pragma once

#include <stdint.h>

class IInputControl
{
  protected:
    int m_id;

  public:
    int GetId() const { return m_id; }
    virtual bool IsActuated() const = 0;
    friend bool operator<(const IInputControl& left, const IInputControl& right);
};

bool operator<(const IInputControl& left, const IInputControl& right)
{
    return left.m_id < right.m_id;
}

template <typename T>
class InputControl : protected IInputControl
{
  protected:
    T m_value;
    T m_defaultValue;

    T GetValue() const
    {
        return m_value;
    }

    void SetValue(T value)
    {
        m_value = value;
    }

  public:
    bool IsActuated() const
    {
        return m_value != m_defaultValue;
    }
};

class KeyControl : protected InputControl<uint8_t>
{
    friend class Keyboard;

    int m_code;

    KeyControl(int code)
    {
        m_defaultValue = 0.;
        m_value = 0.;
        m_code = code;
        m_id = code; // TODO: For now we use GLFW scancodes as ids
    }
};
