#include "input_service.hpp"
#include "input_context.hpp"

#include <GLFW/glfw3.h>

namespace aln
{

void InputService::RegisterContext(InputContext* pContext)
{
    /// todo: prevent duplicates
    m_contexts.push_back(pContext);
}

void InputService::UnregisterContext(InputContext* pContext)
{
    auto iter = std::find(m_contexts.begin(), m_contexts.end(), pContext);
    assert(iter != m_contexts.end()); // Tried to unregister an unregistered context
    m_contexts.erase(iter);
}

void InputService::Dispatch()
{
    // 1. Poll triggered controls from the devices
    HashMap<UUID, ControlStateChangedEvent> events;
    m_keyboard.PollControlChangedEvents(events);
    m_mouse.PollControlChangedEvents(events);
    m_gamepad.PollControlChangedEvents(events);

    // Exit right away if no control change events were raised
    if (events.empty())
    {
        return;
    }
    // 2. TODO: Loop over bindings to find active ones ?

    // 3. Pass events to the interested contexts for consumption
    for (auto pContext : m_contexts)
    {
        assert(pContext != nullptr);

        if (pContext->IsEnabled())
        {
            pContext->Map(events);
        }
    }
}
} // namespace aln