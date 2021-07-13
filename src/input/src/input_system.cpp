#include "input_system.hpp"
#include "input_context.hpp"

namespace aln::input
{
void Input::RegisterContext(InputContext* pContext)
{
    Input& singleton = Input::Singleton();
    singleton.m_contexts.push_back(pContext);
}

const Keyboard& Input::Keyboard()
{
    Input& singleton = Input::Singleton();
    return singleton.m_keyboard;
};

const Mouse& Input::Mouse()
{
    Input& singleton = Input::Singleton();
    return singleton.m_mouse;
}

void Input::Dispatch()
{
    auto& singleton = Singleton();
    // 1. Poll triggered controls from the devices
    auto events = singleton.m_keyboard.PollControlChangedEvents();
    events.merge(singleton.m_mouse.PollControlChangedEvents());
    // controls.merge(Gamepad.PollControlChangedEvents());

    // Exit right away if no control change events were raised
    if (events.empty())
    {
        return;
    }
    // 2. TODO: Loop over bindings to find active ones ?

    // 3. Pass events to the interested contexts for consumption
    for (InputContext* c : singleton.m_contexts)
    {
        assert(c != nullptr);

        if (c->IsEnabled())
        {
            events = c->Map(events);
        }
    }
}

Input& Input::Singleton()
{
    static Input singleton;
    return singleton;
}
}