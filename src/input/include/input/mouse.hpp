#pragma once

#include "controls/axis_control.hpp"
#include "controls/button_control.hpp"
#include "input_device.hpp"

#include <glm/vec2.hpp>
#include <vector>

namespace aln
{
class Engine;
namespace input
{
/// @brief Describe a physical mouse and its on-screen cursor relative.
class Mouse : IInputDevice
{
    friend class InputService;
    friend Engine;

  private:
    // Position in screen space.
    glm::vec2 m_position;

    // Difference in curosr position since last frame.
    glm::vec2 m_delta = {0, 0};

    // Difference in position of the scroller since last frame.
    glm::vec2 m_scrollDelta = {0, 0};

    /// Mouse buttons
    std::map<int, ButtonControl> m_buttons;

    /// Mouse scroll
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
        for (auto& [buttonCode, buttonControl] : m_buttons)
        {
            buttonControl.Update();
        }

        // Reset deltas
        m_scrollDelta = {0.0f, 0.0f};
    }

    /// @brief Translate a GLFW Event to KeyControl
    /// @todo Move to virtual fn in InputDevice (possibly InputControl even ?)
    void UpdateControlState(int code, int action);

    void UpdateScrollControlState(float xdelta, float ydelta);

    const ButtonControl& GetButton(int code) const
    {
        auto iter = m_buttons.find(code);
        if (iter != m_buttons.end())
        {
            return iter->second;
        }

        throw;
    }

  public:
    const int TEMPORARY_SCROLL_ID = 11111;

    /// @brief Default constructor creates standard mouse controls (right and left click, scrolling wheel).
    Mouse();

    inline const glm::vec2& GetPosition() const { return m_position; }
    inline const glm::vec2& GetDelta() const { return m_delta; }
    inline const glm::vec2& GetScrollDelta() const { return m_scrollDelta; };

    inline bool WasPressed(int code) const
    {
        auto& control = GetButton(code);
        return control.WasPressed();
    }

    inline bool WasReleased(int code) const
    {
        auto& control = GetButton(code);
        return control.WasReleased();
    }

    inline bool IsHeld(int code) const
    {
        auto& control = GetButton(code);
        return control.IsHeld();
    }
};
} // namespace input
} // namespace aln