#include "input_system.hpp"
#include "input_context.hpp"

void Input::RegisterContext(InputContext* pContext)
{
    Input& singleton = Input::Singleton();
    singleton.m_contexts.push_back(pContext);
}

void Input::Dispatch()
{

    // 1. Poll triggered controls from the devices
    auto events = Keyboard.PollControlChangedEvents();
    events.merge(Mouse.PollControlChangedEvents());
    // controls.merge(Gamepad.PollControlChangedEvents());

    // Exit right away if no control change events were raised
    if (events.empty())
    {
        return;
    }
    // 2. TODO: Loop over bindings to find active ones ?

    // 3. Pass events to the interested contexts for consumption
    Input& singleton = Input::Singleton();
    for (InputContext* c : singleton.m_contexts)
    {
        assert(c != nullptr);

        if (c->IsEnabled())
        {
            events = c->Map(events);
        }
    }
}

Mouse Input::Mouse;
Keyboard Input::Keyboard;
