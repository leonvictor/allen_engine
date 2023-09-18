#include "input_context.hpp"

#include "callback_context.hpp"
#include "input_action.hpp"
#include "interactions.hpp"

namespace aln
{

std::multimap<int, ControlStateChangedEvent> InputContext::Map(std::multimap<int, ControlStateChangedEvent> inputMap)
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

void InputContext::RegisterCallback(int keyCode, std::function<void(CallbackContext)> callback)
{
    auto action = m_actions.try_emplace(keyCode);
    action.first->second.SetCallback(callback);
}

InputAction& InputContext::AddAction(int keyCode)
{
    return m_actions.try_emplace(keyCode).first->second;
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