#include "input_context.hpp"

#include "callback_context.hpp"
#include "input_action.hpp"
#include "interactions.hpp"

namespace aln
{

void InputContext::Map(HashMap<UUID, ControlStateChangedEvent>& inputMap)
{
    for (auto& [controlID, action] : m_actions)
    {
        // TODO: How do we handle multiple control actuations in the same frame ?
        // Maybe pack them together and let the action decide for itself ?

        auto it = inputMap.find(controlID);
        if (it != inputMap.end())
        {
            auto pControl = it->second.m_pControl;

            action.Trigger({
                .pControl = pControl,
                // TODO: Populate the context with more info.
            });

            inputMap.erase(it);
        }
    }
}

void InputContext::RegisterCallback(const UUID& controlID, std::function<void(CallbackContext)> callback)
{
    auto action = m_actions.try_emplace(controlID);
    action.first->second.SetCallback(callback);
}

InputAction& InputContext::AddAction(const UUID& controlID)
{
    return m_actions.try_emplace(controlID).first->second;
}

void InputContext::Enable()
{
    for (auto it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        it->second.Enable();
    }
    m_enabled = true;
}

void InputContext::Disable()
{
    for (auto it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        it->second.Disable();
    }
    m_enabled = false;
}

bool InputContext::IsEnabled() const { return m_enabled; }
} // namespace aln