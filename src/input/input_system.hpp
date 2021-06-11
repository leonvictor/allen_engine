#pragma once

#include "../utils/singleton.hpp"
#include "callback_context.hpp"
// #include "input_context.hpp"
#include "keyboard.hpp"
#include "mouse.hpp"

#include <vector>

/// TODO: https://www.gamedev.net/blogs/entry/2250186-designing-a-robust-input-handling-system-for-games/

/// @brief Input is the overarching system recording and dispatching input operations.
class InputContext;

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

    static void RegisterContext(InputContext* pContext);

    /// @brief Process registered input by passing them to active contexts.
    /// TODO: Shouldn't be accessible outside the main engine loop
    static void Dispatch();
};
