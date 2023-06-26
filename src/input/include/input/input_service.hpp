#pragma once

#include "callback_context.hpp"
#include "devices/keyboard.hpp"
#include "devices/mouse.hpp"
#include "devices/gamepad.hpp"

#include <vector>

#include <common/services/service.hpp>

namespace aln
{


/// TODO: https://www.gamedev.net/blogs/entry/2250186-designing-a-robust-input-handling-system-for-games/
class InputContext;

/// @brief Input is the overarching system recording and dispatching input operations.
class InputService : public IService
{
    friend class GLFWApplication;
    friend class Engine;

  private:
    // TODO:
    // - Order contexts by priority (only one context can handle an input at a given time)
    // - Register a name to allow manual lookup

    // List of active InputContexts
    std::vector<InputContext*> m_contexts;

    Keyboard m_keyboard;
    Mouse m_mouse;
    Gamepad m_gamepad;

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