#pragma once

#include "../controls/axis_control.hpp"
#include "../controls/button_control.hpp"
#include "../input_device.hpp"

#include <common/containers/array.hpp>
#include <common/containers/hash_map.hpp>
#include <common/maths/vec2.hpp>

#include <map>

namespace aln
{

/// @brief Describe a physical mouse and its on-screen cursor relative.
class Mouse : IInputDevice
{
    friend class InputService;

  public:
    enum class Button : uint8_t
    {
        Left,
        Right,
        Middle,
        Button4,
        Button5,
        Button6,
        Button7,
        Button8,
    };

  private:
    // Position in screen space.
    Vec2 m_position;

    // Difference in cursor position since last frame.
    Vec2 m_delta = Vec2::Zeroes;

    // Difference in position of the scroller since last frame.
    Vec2 m_scrollDelta = Vec2::Zeroes;

    /// Mouse buttons
    Array<ButtonControl, 8> m_buttons;

    /// Mouse scroll
    /// @todo Rename to scroll (or m_buttons to m_buttonControls but match them)
    AxisControl m_scrollControl;

  private:
    /// @brief Update position and delta according to the new provided position.
    void SetCursorPosition(const Vec2& position)
    {
        m_delta = position - m_position;
        m_position = position;
    }

    void Update()
    {
        for (auto& button : m_buttons)
        {
            button.Update();
        }
    }

    void ClearFrameState() override
    {
        // Reset deltas
        m_scrollDelta = {0.0f, 0.0f};
        m_delta = {0.0f, 0.0f};
    }

    void UpdateControlState(const Button& button, const ButtonState& buttonState);

    void UpdateScrollControlState(float xdelta, float ydelta);

  public:
    inline const Vec2& GetPosition() const { return m_position; }
    inline const Vec2& GetDelta() const { return m_delta; }
    inline const Vec2& GetScrollDelta() const { return m_scrollDelta; };

    inline bool WasPressed(Button button) const { return m_buttons[(uint8_t) button].WasPressed(); }
    inline bool WasReleased(Button button) const { return m_buttons[(uint8_t) button].WasReleased(); }
    inline bool IsHeld(Button button) const { return m_buttons[(uint8_t) button].IsHeld(); }
};
} // namespace aln