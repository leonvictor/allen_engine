#pragma once

#include <common/uuid.hpp>

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
  private:
    UUID m_id = UUID::Generate();

  protected:

  public:
    inline const UUID& GetID() const { return m_id; }
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
    InputControl(const T& defaultValue) : m_defaultValue(defaultValue), m_value(defaultValue) {}

    bool IsActuated() const { return m_value != m_defaultValue; }
    const T& GetValue() const { return m_value; }
};
} // namespace aln
