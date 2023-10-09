#pragma once

#include "control_state_event.hpp"
#include "input_action.hpp"

#include <common/containers/hash_map.hpp>

namespace aln
{

// fwd
struct CallbackContext;

/// @brief Input context hold a group of actions dependant on the same context. They are used to prioritize between them, and to allow easier
/// enabling/disabling
class InputContext
{
  private:
    /// Map of actions <ControlID, Action>
    HashMap<UUID, InputAction> m_actions;
    bool m_enabled = false;

  public:
    /// @brief Map input to the registered actions in this context. Successfully mapped input are consumed and removed from the list.
    /// @todo: Shouldn't be accessible
    void Map(HashMap<UUID, ControlStateChangedEvent>& inputMap);

    /// @brief Create a default InputAction with this callback and register it in this context.
    void RegisterCallback(const UUID& controlID, std::function<void(CallbackContext)> callback);

    /// @brief Create a new InputAction for this keyCode and return it.
    InputAction& AddAction(const UUID& controlID);

    /// @brief Enable all actions in this context.
    void Enable();

    /// @brief Disable all actions in this context.
    void Disable();

    bool IsEnabled() const;
};
} // namespace aln