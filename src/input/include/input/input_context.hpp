#pragma once

#include "callback_context.hpp"
#include "control_state_event.hpp"
#include "input_action.hpp"
#include "interactions.hpp"

#include <functional>
#include <map>

namespace aln::input
{
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
    /// TODO: Shouldn't be accessible
    std::multimap<int, ControlStateChangedEvent> Map(std::multimap<int, ControlStateChangedEvent> inputMap)
    {
        for (auto& pair : m_actions)
        {
            // TODO: How do we handle multiple control actuations in the same frame ?
            // Maybe pack them together and let the action decide for itself ?

            auto node = inputMap.extract(pair.first);
            while (!node.empty())
            {
                InputAction::Context actionContext = {
                    .pControl = node.mapped().pControl,
                    // TODO: Populate the context with more info.
                };

                pair.second.Trigger(actionContext);

                // Extract the next event launched by the associated control
                node = inputMap.extract(pair.first);
            }
        }
        return inputMap;
    }

    /// @brief Create a default InputAction with this callback and register it in this context.
    void RegisterCallback(int keyCode, std::function<void(CallbackContext)> callback)
    {
        auto action = m_actions.emplace(std::make_pair(keyCode, InputAction()));
        action.first->second.SetCallback(callback);
    }

    /// @brief Create a new InputAction for this keyCode and return it.
    InputAction& AddAction(int keyCode)
    {
        auto action = m_actions.emplace(std::make_pair(keyCode, InputAction()));
        return action.first->second;
    }

    /// @brief Enable all actions in this context.
    void Enable()
    {
        for (auto it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            it->second.Enable();
        }
        m_enabled = true;
    }

    /// @brief Disable all actions in this context.
    void Disable()
    {
        for (auto it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            it->second.Disable();
        }
        m_enabled = false;
    }

    bool IsEnabled() const { return m_enabled; }
};
} // namespace aln::input