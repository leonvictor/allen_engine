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
    friend class Input;
    friend Engine;

  private:
    // Position in screen space.
    glm::vec2 m_position;

    // Difference in curosr position since last frame.
    glm::vec2 m_delta;

    // Difference in position of the scroller since last frame.
    glm::vec2 m_scrollDelta;

    /// Mouse buttons
    std::map<int, ButtonControl> m_buttons;

    /// Mouse scroll
    AxisControl m_scrollControl;

    /// @brief Update position and delta according to the new provided position.
    void Update(glm::vec2 position);

    /// @brief Return a list of state changed events that occured since the last call to this function.
    /// TODO: Share this behavior with Keyboard (and other devices)
    /// This probably means moving m_buttons/m_keys to a common m_control
    std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() override;

    /// @brief Translate a GLFW Event to KeyControl
    /// @todo Move to virtual fn in InputDevice (possibly InputControl even ?)
    void UpdateControlState(int code, int action);

    void UpdateScrollControlState(float xdelta, float ydelta);

  public:
    const int TEMPORARY_SCROLL_ID = 11111;

    /// @brief Default constructor creates standard mouse controls (right and left click, scrolling wheel).
    Mouse();

    const glm::vec2& GetPosition() const;
    const glm::vec2& GetDelta() const;
    const glm::vec2& GetScroll() const;
};
} // namespace input
} // namespace aln