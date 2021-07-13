#pragma once

#include "control_state_event.hpp"
#include "controls/key_control.hpp"
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
    friend class Input; // TODO: Add an indirection level to let input handle this

  private:
    /// Map of keyboard keys controls.
    /// @note We use a map as sets elements cannot be modified in place (see https://en.cppreference.com/w/cpp/container/unordered_set)
    std::map<int, KeyControl> m_keys;

    std::multimap<int, ControlStateChangedEvent> m_statesChanged;

    /// @brief Return a list of state changed events that occured since the last call to this function.
    std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() override;

    /// @brief Translate a GLFW Event to KeyControl
    /// @todo Move to virtual fn in InputDevice (possibly InputControl even ?)
    void UpdateControlState(int code, int action);

  public:
    /// @brief Query the keyboard to get the control associated to a key.
    /// @param code: Requested key code
    KeyControl& GetKey(int code);
};
} // namespace input
} // namespace aln