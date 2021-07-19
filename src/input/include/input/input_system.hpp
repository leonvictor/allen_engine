#pragma once

#include "callback_context.hpp"
#include "keyboard.hpp"
#include "mouse.hpp"

#include <vector>

namespace aln
{

class Engine;

namespace input
{

/// TODO: https://www.gamedev.net/blogs/entry/2250186-designing-a-robust-input-handling-system-for-games/
class InputContext;

/// @brief Input is the overarching system recording and dispatching input operations.
class Input
{
    friend aln::Engine;

  private:
    // TODO:
    // - Order contexts by priority (only one context can handle an input at a given time)
    // - Use shared_ptrs so that we can create contexts on the fly ?
    // - Register a name to allow manual lookup

    // List of active InputContexts
    std::vector<InputContext*> m_contexts;

    Keyboard m_keyboard;
    Mouse m_mouse;

    // TODO: Merge in a single function and handle device internally
    static void UpdateKeyboardControlState(int code, int action)
    {
        Singleton().m_keyboard.UpdateControlState(code, action);
    }

    static void UpdateMouseControlState(int code, int action)
    {
        Singleton().m_mouse.UpdateControlState(code, action);
    }

    static void UpdateScrollControlState(int xoffset, int yoffset)
    {
        Singleton().m_mouse.UpdateScrollControlState(xoffset, yoffset);
    }

    static void UpdateMousePosition(glm::vec2 position)
    {
        Singleton().m_mouse.Update(position);
    }

    static Input& Singleton();

  public:
    static void RegisterContext(InputContext* pContext);
    static void UnregisterContext(InputContext* pContext);

    /// @brief Process registered input by passing them to active contexts.
    /// TODO: Shouldn't be accessible outside the main engine loop
    static void Dispatch();

    // TODO: Handle multiple device of each type.
    /// @brief Static access to the keyboard.
    static const Keyboard& Keyboard();
    /// @brief Static access to the mouse.
    static const Mouse& Mouse();

    /// TODO: Static access to the gamepad.
    // static const Gamepad& Gamepad();
};
} // namespace input
} // namespace aln