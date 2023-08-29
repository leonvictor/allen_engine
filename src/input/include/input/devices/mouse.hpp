#pragma once

#include "controls/axis_control.hpp"
#include "controls/button_control.hpp"
#include "input_device.hpp"

#include <glm/vec2.hpp>

#include <array>
#include <unordered_map>
#include <vector>

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
    static const std::unordered_map<uint8_t, Button> GlfwButtonMap;

    // Position in screen space.
    glm::vec2 m_position;

    // Difference in cursor position since last frame.
    glm::vec2 m_delta = {0, 0};

    // Difference in position of the scroller since last frame.
    glm::vec2 m_scrollDelta = {0, 0};

    /// Mouse buttons
    std::array<ButtonControl, 8> m_buttons;

    /// Mouse scroll
    /// @todo Rename to scroll (or m_buttons to m_buttonControls but match them)
    AxisControl m_scrollControl;

    /// @brief Update position and delta according to the new provided position.
    void SetCursorPosition(glm::vec2 position)
    {
        m_delta = position - m_position;
        m_position = position;
    }

    /// @brief Return a list of state changed events that occured since the last call to this function.
    /// TODO: Share this behavior with Keyboard (and other devices)
    /// This probably means moving m_buttons/m_keys to a common m_control
    std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() override;

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

    /// @brief Translate a GLFW Event to KeyControl
    void UpdateControlState(int code, int action);

    void UpdateScrollControlState(float xdelta, float ydelta);

  public:
    inline const glm::vec2& GetPosition() const { return m_position; }
    inline const glm::vec2& GetDelta() const { return m_delta; }
    inline const glm::vec2& GetScrollDelta() const { return m_scrollDelta; };

    inline bool WasPressed(Button button) const { return m_buttons[(uint8_t) button].WasPressed(); }
    inline bool WasReleased(Button button) const { return m_buttons[(uint8_t) button].WasReleased(); }
    inline bool IsHeld(Button button) const { return m_buttons[(uint8_t) button].IsHeld(); }
};
} // namespace aln