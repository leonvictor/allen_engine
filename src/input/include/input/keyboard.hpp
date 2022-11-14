#pragma once

#include "control_state_event.hpp"
#include "controls/button_control.hpp"
#include "input_device.hpp"

#include <glm/vec2.hpp>
#include <map>

namespace aln
{
class Engine;
namespace input
{

/// @brief Describes a physical keyboard.
class Keyboard : public IInputDevice
{
    friend aln::Engine;
    friend class InputService; // TODO: Add an indirection level to let input handle this

  private:
    /// Map of keyboard keys controls.
    /// @note We use a map as sets elements cannot be modified in place (see https://en.cppreference.com/w/cpp/container/unordered_set)
    std::map<int, ButtonControl> m_keys;

    std::multimap<int, ControlStateChangedEvent> m_statesChanged;

    /// @brief Return a list of state changed events that occured since the last call to this function.
    std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() override;

    /// @brief Query the keyboard to get the control associated to a key.
    /// @param code: Requested key code
    inline const ButtonControl& GetKey(int code) const;

    /// @brief Translate a GLFW Event to ButtonControl
    /// @todo Move to virtual fn in InputDevice (possibly InputControl even ?)
    void UpdateControlState(int code, int action);

    void Update()
    {
        for (auto& [keyIndex, keyControl] : m_keys)
        {
            keyControl.Update();
        }
    }

  public:
    inline bool WasPressed(int code) const
    {
        auto& control = GetKey(code);
        return control.WasPressed();
    }

    inline bool WasReleased(int code) const
    {
        auto& control = GetKey(code);
        return control.WasReleased();
    }

    inline bool IsHeld(int code) const
    {
        auto& control = GetKey(code);
        return control.IsHeld();
    }
};
} // namespace input
} // namespace aln