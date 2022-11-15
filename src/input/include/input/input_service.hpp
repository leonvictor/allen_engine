#pragma once

#include "callback_context.hpp"
#include "keyboard.hpp"
#include "mouse.hpp"

#include <vector>

#include <common/services/service.hpp>

namespace aln
{

class Engine;

/// TODO: https://www.gamedev.net/blogs/entry/2250186-designing-a-robust-input-handling-system-for-games/
class InputContext;

/// @brief Input is the overarching system recording and dispatching input operations.
class InputService : public IService
{
    friend aln::Engine;

  private:
    // TODO:
    // - Order contexts by priority (only one context can handle an input at a given time)
    // - Register a name to allow manual lookup

    // List of active InputContexts
    std::vector<InputContext*> m_contexts;

    Keyboard m_keyboard;
    Mouse m_mouse;

    // TODO: Merge in a single function and handle device internally
    void UpdateKeyboardControlState(int code, int action)
    {
        m_keyboard.UpdateControlState(code, action);
    }

    void UpdateMouseControlState(int code, int action)
    {
        m_mouse.UpdateControlState(code, action);
    }

    void UpdateScrollControlState(float xoffset, float yoffset)
    {
        m_mouse.UpdateScrollControlState(xoffset, yoffset);
    }

    /// @todo Should not be updated through the main loop. Pass the glfw window ?
    void UpdateMousePosition(glm::vec2 position)
    {
        m_mouse.SetCursorPosition(position);
    }

    /// @brief Update all input devices
    void Update()
    {
        m_keyboard.Update();
        m_mouse.Update();
        // m_gamepad.Update();

        Dispatch();
    }

    /// @brief Process registered input by passing them to active contexts.
    void Dispatch();

  public:
    void RegisterContext(InputContext* pContext);
    void UnregisterContext(InputContext* pContext);

    // TODO: Handle multiple device of each type.
    /// @brief Access to the keyboard.
    inline const Keyboard* GetKeyboard() const { return &m_keyboard; }
    /// @brief Access to the mouse.
    inline const Mouse* GetMouse() const { return &m_mouse; }

    /// TODO: Access to the gamepad.
    // static const GetGamepad& Gamepad();
};
} // namespace aln