#include "devices/keyboard.hpp"

#include "control_state_event.hpp"
#include "input_device.hpp"

#include <GLFW/glfw3.h>

#include <map>

namespace aln
{

// 1:1 Map from glfw key codes to ours
const HashMap<uint16_t, Keyboard::Key> Keyboard::GlfwKeyMap = {
    {GLFW_KEY_UNKNOWN, Key::Unknown},
    {GLFW_KEY_SPACE, Key::Space},
    {GLFW_KEY_APOSTROPHE, Key::Apostrophe}, /* ' */
    {GLFW_KEY_COMMA, Key::Comma},           /* , */
    {GLFW_KEY_MINUS, Key::Minus},           /* - */
    {GLFW_KEY_PERIOD, Key::Period},         /* . */
    {GLFW_KEY_SLASH, Key::Slash},           /* / */
    {GLFW_KEY_0, Key::Alpha0},
    {GLFW_KEY_1, Key::Alpha1},
    {GLFW_KEY_2, Key::Alpha2},
    {GLFW_KEY_3, Key::Alpha3},
    {GLFW_KEY_4, Key::Alpha4},
    {GLFW_KEY_5, Key::Alpha5},
    {GLFW_KEY_6, Key::Alpha6},
    {GLFW_KEY_7, Key::Alpha7},
    {GLFW_KEY_8, Key::Alpha8},
    {GLFW_KEY_9, Key::Alpha9},
    {GLFW_KEY_SEMICOLON, Key::Semicolon}, /* ; */
    {GLFW_KEY_EQUAL, Key::Equal},         /* = */
    {GLFW_KEY_A, Key::A},
    {GLFW_KEY_B, Key::B},
    {GLFW_KEY_C, Key::C},
    {GLFW_KEY_D, Key::D},
    {GLFW_KEY_E, Key::E},
    {GLFW_KEY_F, Key::F},
    {GLFW_KEY_G, Key::G},
    {GLFW_KEY_H, Key::H},
    {GLFW_KEY_I, Key::I},
    {GLFW_KEY_J, Key::J},
    {GLFW_KEY_K, Key::K},
    {GLFW_KEY_L, Key::L},
    {GLFW_KEY_M, Key::M},
    {GLFW_KEY_N, Key::N},
    {GLFW_KEY_O, Key::O},
    {GLFW_KEY_P, Key::P},
    {GLFW_KEY_Q, Key::Q},
    {GLFW_KEY_R, Key::R},
    {GLFW_KEY_S, Key::S},
    {GLFW_KEY_T, Key::T},
    {GLFW_KEY_U, Key::U},
    {GLFW_KEY_V, Key::V},
    {GLFW_KEY_W, Key::W},
    {GLFW_KEY_X, Key::X},
    {GLFW_KEY_Y, Key::Y},
    {GLFW_KEY_Z, Key::Z},
    {GLFW_KEY_LEFT_BRACKET, Key::LeftBracket},   /* [ */
    {GLFW_KEY_BACKSLASH, Key::Backslash},        /* \ */
    {GLFW_KEY_RIGHT_BRACKET, Key::RightBracket}, /* ] */
    {GLFW_KEY_GRAVE_ACCENT, Key::GraveAccent},   /* ` */
    {GLFW_KEY_WORLD_1, Key::World1},             /* non-US #1 */
    {GLFW_KEY_WORLD_2, Key::World2},             /* non-US #2 */
    {GLFW_KEY_ESCAPE, Key::Escape},
    {GLFW_KEY_ENTER, Key::Enter},
    {GLFW_KEY_TAB, Key::Tab},
    {GLFW_KEY_BACKSPACE, Key::Backspace},
    {GLFW_KEY_INSERT, Key::Insert},
    {GLFW_KEY_DELETE, Key::Delete},
    {GLFW_KEY_RIGHT, Key::RightArrow},
    {GLFW_KEY_LEFT, Key::LeftArrow},
    {GLFW_KEY_DOWN, Key::DownArrow},
    {GLFW_KEY_UP, Key::KeyUp},
    {GLFW_KEY_PAGE_UP, Key::PageUp},
    {GLFW_KEY_PAGE_DOWN, Key::PageDown},
    {GLFW_KEY_HOME, Key::Home},
    {GLFW_KEY_END, Key::End},
    {GLFW_KEY_CAPS_LOCK, Key::CapsLock},
    {GLFW_KEY_SCROLL_LOCK, Key::ScrollLock},
    {GLFW_KEY_NUM_LOCK, Key::NumLock},
    {GLFW_KEY_PRINT_SCREEN, Key::PrintScreen},
    {GLFW_KEY_PAUSE, Key::Pause},
    {GLFW_KEY_F1, Key::F1},
    {GLFW_KEY_F2, Key::F2},
    {GLFW_KEY_F3, Key::F3},
    {GLFW_KEY_F4, Key::F4},
    {GLFW_KEY_F5, Key::F5},
    {GLFW_KEY_F6, Key::F6},
    {GLFW_KEY_F7, Key::F7},
    {GLFW_KEY_F8, Key::F8},
    {GLFW_KEY_F9, Key::F9},
    {GLFW_KEY_F10, Key::F10},
    {GLFW_KEY_F11, Key::F11},
    {GLFW_KEY_F12, Key::F12},
    {GLFW_KEY_F13, Key::F13},
    {GLFW_KEY_F14, Key::F14},
    {GLFW_KEY_F15, Key::F15},
    {GLFW_KEY_F16, Key::F16},
    {GLFW_KEY_F17, Key::F17},
    {GLFW_KEY_F18, Key::F18},
    {GLFW_KEY_F19, Key::F19},
    {GLFW_KEY_F20, Key::F20},
    {GLFW_KEY_F21, Key::F21},
    {GLFW_KEY_F22, Key::F22},
    {GLFW_KEY_F23, Key::F23},
    {GLFW_KEY_F24, Key::F24},
    {GLFW_KEY_F25, Key::F25},
    {GLFW_KEY_KP_0, Key::Keypad0},
    {GLFW_KEY_KP_1, Key::Keypad1},
    {GLFW_KEY_KP_2, Key::Keypad2},
    {GLFW_KEY_KP_3, Key::Keypad3},
    {GLFW_KEY_KP_4, Key::Keypad4},
    {GLFW_KEY_KP_5, Key::Keypad5},
    {GLFW_KEY_KP_6, Key::Keypad6},
    {GLFW_KEY_KP_7, Key::Keypad7},
    {GLFW_KEY_KP_8, Key::Keypad8},
    {GLFW_KEY_KP_9, Key::Keypad9},
    {GLFW_KEY_KP_DECIMAL, Key::KeypadDecimal},
    {GLFW_KEY_KP_DIVIDE, Key::KeypadDivide},
    {GLFW_KEY_KP_MULTIPLY, Key::KeypadMultiply},
    {GLFW_KEY_KP_SUBTRACT, Key::KeypadSubstract},
    {GLFW_KEY_KP_ADD, Key::KeypadAdd},
    {GLFW_KEY_KP_ENTER, Key::KeypadEnter},
    {GLFW_KEY_KP_EQUAL, Key::KeypadEqual},
    {GLFW_KEY_LEFT_SHIFT, Key::LeftShift},
    {GLFW_KEY_LEFT_CONTROL, Key::LeftControl},
    {GLFW_KEY_LEFT_ALT, Key::LeftAlt},
    {GLFW_KEY_LEFT_SUPER, Key::LeftSuper},
    {GLFW_KEY_RIGHT_SHIFT, Key::RightShift},
    {GLFW_KEY_RIGHT_CONTROL, Key::RightControl},
    {GLFW_KEY_RIGHT_ALT, Key::RightAlt},
    {GLFW_KEY_RIGHT_SUPER, Key::RightSuper},
    {GLFW_KEY_MENU, Key::Menu},
};

std::multimap<int, ControlStateChangedEvent> Keyboard::PollControlChangedEvents()
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
    // for (auto iter = m_keys.begin(); iter != m_keys.end(); ++iter)
    auto numKeys = m_keys.size();
    for (uint16_t keyIdx = 0; keyIdx < numKeys; ++keyIdx)
    {
        // Populate the list
        if (!m_statesChanged.contains(keyIdx) && m_keys[keyIdx].IsActuated())
        {
            auto& event = m_statesChanged.emplace(std::make_pair(keyIdx, ControlStateChangedEvent()))->second;
            event.pControl = &m_keys[keyIdx];
        }
    }

    // TODO: Not sure that works
    std::multimap<int, ControlStateChangedEvent> clone;
    clone.merge(m_statesChanged);

    m_statesChanged.clear();
    return clone;
}

void Keyboard::UpdateControlState(int glfwCode, int action)
{
    // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
    if (action == GLFW_REPEAT)
    {
        return;
    }

    // TODO: Add buttons statically once at app launch and use map.find here
    // Find the control if it has already been added to the device, create it otherwise
    // auto iter = m_keys.emplace(std::make_pair(code, ButtonControl(code))).first;
    auto iter = GlfwKeyMap.find(glfwCode);
    assert(iter != GlfwKeyMap.end());

    // Update the control value
    if (action == GLFW_PRESS)
    {
        m_keys[(uint16_t) iter->second].SetValue(ButtonState::Pressed);
    }

    else if (action == GLFW_RELEASE)
    {
        m_keys[(uint16_t) iter->second].SetValue(ButtonState::Released);
    }

    // Create and populate a control state changed event
    auto& event = m_statesChanged.emplace(std::make_pair(glfwCode, ControlStateChangedEvent()))->second;
    event.pControl = &m_keys[(uint16_t) iter->second];
}
} // namespace aln