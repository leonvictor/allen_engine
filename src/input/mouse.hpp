#pragma once

#include "input_device.hpp"

#include <glm/vec2.hpp>
#include <vector>

/// @brief Describe a physical mouse and its on-screen cursor relative.
class Mouse : InputDevice
{
    friend class Engine;

  private:
    // Position in screen space.
    glm::vec2 m_position;
    // Difference in position since last frame.
    glm::vec2 m_delta;

    std::vector<bool> m_buttons;
    // Difference in position of the scroller since last frame.
    glm::vec2 m_scrollDelta;

    /// @brief Update position and delta according to the new provided position.
    void Update(glm::vec2 position)
    {
        m_delta = position - m_position;
        m_position = position;
    }

    // TODO: Because GLFW only triggers KEY_PRESSED and KEY_RELEASE events,
    // we will need to manually loop over the keys to lauch repeat events as well.
    // (On polling)
  public:
    glm::vec2 GetPosition() { return m_position; }
    glm::vec2 GetDelta() { return m_delta; }
    glm::vec2 GetScroll() { return m_scrollDelta; }
};