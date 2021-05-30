#pragma once

#include <stdint.h>

class IInputControl
{
  protected:
    /// TODO: Use GUID
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
class InputControl : public IInputControl
{
    friend class IInputDevice;

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
