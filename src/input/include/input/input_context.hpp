#pragma once

#include "control_state_event.hpp"

#include <functional>
#include <map>

namespace aln::input
{

// fwd
class InputAction;
class CallbackContext;

/// @brief Input context hold a group of actions dependant on the same context. They are used to prioritize between them, and to allow easier
/// enabling/disabling
class InputContext
{
  private:
    /// Map of actions <ControlID, Action>
    std::map<int, InputAction> m_actions;
    bool m_enabled = false;

  public:
    /// @brief Map input to the registered actions in this context. Successfully mapped input are consumed and removed from the list.
    /// @todo: Shouldn't be accessible
    std::multimap<int, ControlStateChangedEvent> Map(std::multimap<int, ControlStateChangedEvent> inputMap);

    /// @brief Create a default InputAction with this callback and register it in this context.
    void RegisterCallback(int keyCode, std::function<void(CallbackContext)> callback);

    /// @brief Create a new InputAction for this keyCode and return it.
    InputAction& AddAction(int keyCode);

    /// @brief Enable all actions in this context.
    void Enable();

    /// @brief Disable all actions in this context.
    void Disable();

    bool IsEnabled() const;
};
} // namespace aln::input