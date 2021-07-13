#pragma once

#include <stdint.h>

class IInputControl
{
  protected:
    /// TODO: Use GUID
    int m_id;

  public:
    inline int GetId() const { return m_id; }
    virtual bool IsActuated() const = 0;
    friend bool operator<(const IInputControl& left, const IInputControl& right);
};

bool operator<(const IInputControl& left, const IInputControl& right);

namespace input::devices
{
class Keyboard;
class Mouse;
} // namespace input::devices

template <typename T>
class InputControl : public IInputControl
{
    friend class IInputDevice;
    friend class input::devices::Keyboard;
    friend class input::devices::Mouse;

  protected:
    T m_value;
    T m_defaultValue;

    void SetValue(T value)
    {
        m_value = value;
    }

  public:
    bool IsActuated() const
    {
        return m_value != m_defaultValue;
    }

    T GetValue() const
    {
        return m_value;
    }
};
