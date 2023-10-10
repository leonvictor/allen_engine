#pragma once

#include "../control_state_event.hpp"
#include "../controls/button_control.hpp"
#include "../input_device.hpp"

#include <common/containers/array.hpp>
#include <common/containers/hash_map.hpp>

namespace aln
{
class Engine;

/// @brief Describes a physical keyboard.
class Keyboard : public IInputDevice
{
    friend Engine;
    friend class InputService;

  public:
    enum class Key : uint16_t
    {
        Unknown,
        Space,
        Apostrophe,
        Comma,
        Minus,
        Period,
        Slash,
        Alpha0,
        Alpha1,
        Alpha2,
        Alpha3,
        Alpha4,
        Alpha5,
        Alpha6,
        Alpha7,
        Alpha8,
        Alpha9,
        Semicolon,
        Equal,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        LeftBracket,
        Backslash,
        RightBracket,
        GraveAccent,
        World1,
        World2,
        Escape,
        Enter,
        Tab,
        Backspace,
        Insert,
        Delete,
        RightArrow,
        LeftArrow,
        DownArrow,
        KeyUp,
        PageUp,
        PageDown,
        Home,
        End,
        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        F25,
        Keypad0,
        Keypad1,
        Keypad2,
        Keypad3,
        Keypad4,
        Keypad5,
        Keypad6,
        Keypad7,
        Keypad8,
        Keypad9,
        KeypadDecimal,
        KeypadDivide,
        KeypadMultiply,
        KeypadSubstract,
        KeypadAdd,
        KeypadEnter,
        KeypadEqual,
        LeftShift,
        LeftControl,
        LeftAlt,
        LeftSuper,
        RightShift,
        RightControl,
        RightAlt,
        RightSuper,
        Menu,
    };

  private:
    Array<ButtonControl, 131> m_keys;

    /// @brief Translate a GLFW Event to ButtonControl
    void UpdateControlState(const Keyboard::Key& key, const ButtonState& buttonState);

    void Update() override
    {
        for (auto& key : m_keys)
        {
            key.Update();
        }
    }

  public:
    // ----- Polling API
    inline bool WasPressed(Key key) const { return m_keys[(uint16_t) key].WasPressed(); }
    inline bool WasReleased(Key key) const { return m_keys[(uint16_t) key].WasReleased(); }
    inline bool IsHeld(Key key) const { return m_keys[(uint16_t) key].IsHeld(); }
};
} // namespace aln