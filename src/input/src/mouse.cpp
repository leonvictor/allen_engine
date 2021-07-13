#include "mouse.hpp"
#include "control_state_event.hpp"

#include <GLFW/glfw3.h>

namespace aln::input
{

void Mouse::Update(glm::vec2 position)
{
    m_delta = position - m_position;
    m_position = position;
}

/// @brief Return a list of state changed events that occured since the last call to this function.
/// TODO: Share this behavior with Keyboard (and other devices)
/// This probably means moving m_buttons/m_keys to a common m_control
std::multimap<int, ControlStateChangedEvent> Mouse::PollControlChangedEvents()
{
    // TODO: How do I handle multiple changes per frame ?
    // Scenarios:
    // 1. Loop over all keys when polling to get the actuated ones
    //      -> We're missing just released ones
    // 2. Update the event list when receiving glfw events and complete it when polling
    //      -> Ok, but only with a simple map, so a single change per frame
    // 3. Same as 2. but only update the ones that are not yet in the event list
    //      -> Yeah sure. Can we do this without unnecesary loops ?

    // This is 3.
    /// FIXME: Too many loops
    for (auto iter = m_buttons.begin(); iter != m_buttons.end(); ++iter)
    {
        // Populate the list
        if (!m_statesChanged.contains(iter->first) && iter->second.IsActuated())
        {
            auto& event = m_statesChanged.emplace(std::make_pair(iter->first, ControlStateChangedEvent()))->second;
            event.pControl = &iter->second;
        }
    }

    std::multimap<int, ControlStateChangedEvent> clone = m_statesChanged;
    m_statesChanged.clear();
    return clone;
}

void Mouse::UpdateControlState(int code, int action)
{
    // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
    if (action == GLFW_REPEAT)
    {
        return;
    }

    // Find the control if it has already been added to the device, create it otherwise
    auto iter = m_buttons.emplace(std::make_pair(code, ButtonControl(code))).first;
    // Update the control value
    iter->second.SetValue((float) MapGLFWActionCode(action));

    // Create and populate a control state changed event
    auto& event = m_statesChanged.emplace(std::make_pair(code, ControlStateChangedEvent()))->second;
    event.pControl = &iter->second;
}

void Mouse::UpdateScrollControlState(float xdelta, float ydelta)
{
    m_scrollControl.SetValue(ydelta);
    auto& event = m_statesChanged.emplace(std::make_pair(TEMPORARY_SCROLL_ID, ControlStateChangedEvent()))->second;
    event.pControl = &m_scrollControl;

    m_scrollDelta = glm::vec2(xdelta, ydelta);
}

Mouse::Mouse()
{
    m_buttons.emplace(std::make_pair(GLFW_MOUSE_BUTTON_1, ButtonControl(GLFW_MOUSE_BUTTON_1)));
    m_buttons.emplace(std::make_pair(GLFW_MOUSE_BUTTON_2, ButtonControl(GLFW_MOUSE_BUTTON_2)));
    m_buttons.emplace(std::make_pair(GLFW_MOUSE_BUTTON_3, ButtonControl(GLFW_MOUSE_BUTTON_3)));

    // FIXME: ID should be unique
    m_scrollControl = AxisControl(TEMPORARY_SCROLL_ID);
}

const glm::vec2& Mouse::GetPosition() const { return m_position; }
const glm::vec2& Mouse::GetDelta() const { return m_delta; }
const glm::vec2& Mouse::GetScroll() const { return m_scrollDelta; }
}