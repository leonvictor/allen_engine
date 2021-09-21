#pragma once

#include <map>

namespace aln::input
{
struct ControlStateChangedEvent;

class IInputDevice
{
  protected:
    std::multimap<int, ControlStateChangedEvent> m_statesChanged;

    /// @brief: Map a GLFW action code to a keyboard control value.
    /// TODO: Externalize this to an abstract mapping interface
    uint8_t MapGLFWActionCode(int action);

  public:
    /// @brief Return a list of state changed events that occured since the last call to this method.
    virtual std::multimap<int, ControlStateChangedEvent> PollControlChangedEvents() = 0;

    // /// @brief Translate a GLFW Event to KeyControl
    // // TODO: Move to virtual fn in InputDevice (possibly InputControl even ?)
    // template <template <typename V> typename T>
    // void UpdateControlState(int code, int action)
    // {
    //     // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
    //     if (action == GLFW_REPEAT)
    //     {
    //         return;
    //     }

    //     // Find the control if it has already been added to the device, create it otherwise
    //     auto iter = m_buttons.emplace(std::make_pair(code, T(code))).first;
    //     // Update the control value
    //     iter->second.SetValue(static_cast<V>(MapGLFWActionCode(action)));

    //     // Create and populate a control state changed event
    //     auto& event = m_statesChanged.emplace(std::make_pair(code, ControlStateChangedEvent()))->second;
    //     event.pControl = &iter->second;
    // }

    // virtual void UpdateControlState(int code, int action) = 0;
};
} // namespace aln::input