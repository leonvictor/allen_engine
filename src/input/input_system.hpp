#pragma once

#include "../utils/singleton.hpp"
#include "callback_context.hpp"
#include "input_context.hpp"
#include "keyboard.hpp"
#include "mouse.hpp"

#include <GLFW/glfw3.h>
#include <set>
#include <vector>

/// TODO: https://www.gamedev.net/blogs/entry/2250186-designing-a-robust-input-handling-system-for-games/
/// TODO: Rename to Input ?

/// @brief Input is the overarching system recording and dispatching input operations.
class Input : private ISingleton<Input>
{
  private:
    // TODO:
    // - Order contexts by priority (only one context can handle an input at a given time)
    // - Use shared_ptrs so that we can create contexts on the fly ?
    // - Register a name to allow manual lookup

    // List of active InputContexts
    std::vector<InputContext*> m_contexts;

  public:
    // TODO: Handle multiple device of each type.
    /// Static access to the mouse.
    static Mouse Mouse;
    /// Static access to the keyboard.
    static Keyboard Keyboard;
    /// Static access to the gamepad.
    // static Gamepad Gamepad;

    static void RegisterContext(InputContext* pContext)
    {
        Input& singleton = Input::Singleton();
        singleton.m_contexts.push_back(pContext);
    };

    /// @brief Process registered input by passing them to active contexts.
    /// TODO: Shouldn't be accessible outside the main engine loop
    static void Dispatch()
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
};

Mouse Input::Mouse;
Keyboard Input::Keyboard;
