#include <glfw/glfw3.h>

#include "input_device.hpp"

namespace aln::input
{
uint8_t IInputDevice::MapGLFWActionCode(int action)
{
    if (action == GLFW_PRESS)
    {
        return 1;
    }

    else if (action == GLFW_RELEASE)
    {
        return 0;
    }

    // TODO: Specific error
    throw;
}

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
}