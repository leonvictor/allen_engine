#pragma once

#include <stdint.h>

namespace aln
{
enum class ButtonState : uint8_t
{
    None,
    Pressed,
    Held,
    Released
};

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

template <typename T>
class InputControl : public IInputControl
{
    friend class IInputDevice;
    friend class Keyboard;
    friend class Mouse;

  public:
    enum class UpdateState : uint8_t
    {
        None,
        Changed,        // Control was touched this frame
        RequiresUpdate, // Control requires an update
    };

  protected:
    T m_value;
    T m_defaultValue;
    UpdateState m_updateState = UpdateState::None;

    void SetValue(T value)
    {
        m_value = value;
        m_updateState = UpdateState::Changed;
    }

    virtual void Update() = 0;

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
} // namespace aln
