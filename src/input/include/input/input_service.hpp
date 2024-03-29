#pragma once

#include "callback_context.hpp"
#include "devices/keyboard.hpp"
#include "devices/mouse.hpp"
#include "devices/gamepad.hpp"

#include <common/containers/vector.hpp>
#include <common/services/service.hpp>

namespace aln
{


/// TODO: https://www.gamedev.net/blogs/entry/2250186-designing-a-robust-input-handling-system-for-games/
class InputContext;

/// @brief Input is the overarching system recording and dispatching input operations.
class InputService : public IService
{
    friend class GLFWApplication;
    friend class GLFWInputMapper;
    friend class Engine;

  private:
    // TODO:
    // - Order contexts by priority (only one context can handle an input at a given time)
    // - Register a name to allow manual lookup

    // List of active InputContexts
    Vector<InputContext*> m_contexts;

    Keyboard m_keyboard;
    Mouse m_mouse;
    Gamepad m_gamepad;

    // TODO: Merge in a single function and handle device internally
    void UpdateKeyboardControlState(const Keyboard::Key& key, const ButtonState& buttonState)
    {
        m_keyboard.UpdateControlState(key, buttonState);
    }

    void UpdateMouseControlState(const Mouse::Button& button, const ButtonState& buttonState)
    {
        m_mouse.UpdateControlState(button, buttonState);
    }

    void UpdateScrollControlState(float xoffset, float yoffset)
    {
        m_mouse.UpdateScrollControlState(xoffset, yoffset);
    }

    /// @todo Should not be updated through the main loop. Pass the glfw window ?
    void UpdateMousePosition(Vec2 position)
    {
        m_mouse.SetCursorPosition(position);
    }

    /// @brief Update all input devices
    void Update()
    {
        m_keyboard.Update();
        m_mouse.Update();
        m_gamepad.Update();

        Dispatch();
    }

    void ClearFrameState()
    {
        m_keyboard.ClearFrameState();
        m_mouse.ClearFrameState();
        //m_gamepad.ClearFrameState();
    }

    /// @brief Process registered input by passing them to active contexts.
    void Dispatch();

  public:
    void RegisterContext(InputContext* pContext);
    void UnregisterContext(InputContext* pContext);

    // TODO: Handle multiple device of each type (or none).
    /// @brief Access to the keyboard.
    inline const Keyboard* GetKeyboard() const { return &m_keyboard; }
    /// @brief Access to the mouse.
    inline const Mouse* GetMouse() const { return &m_mouse; }

    inline const Gamepad* GetGamepad() const { return &m_gamepad; }
};
} // namespace aln