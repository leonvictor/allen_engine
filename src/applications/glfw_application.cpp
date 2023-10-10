#pragma once

#include "engine.hpp"

#include <common/memory.hpp>
#include <graphics/device.hpp>
#include <graphics/instance.hpp>
#include <graphics/swapchain.hpp>

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

namespace aln
{

class GLFWInputMapper
{
  private:
    InputService* m_pInputService = nullptr;

    inline static HashMap<int, Keyboard::Key> KeyboardKeyMapping = {
        {GLFW_KEY_UNKNOWN, Keyboard::Key::Unknown},
        {GLFW_KEY_SPACE, Keyboard::Key::Space},
        {GLFW_KEY_APOSTROPHE, Keyboard::Key::Apostrophe}, /* ' */
        {GLFW_KEY_COMMA, Keyboard::Key::Comma},           /* , */
        {GLFW_KEY_MINUS, Keyboard::Key::Minus},           /* - */
        {GLFW_KEY_PERIOD, Keyboard::Key::Period},         /* . */
        {GLFW_KEY_SLASH, Keyboard::Key::Slash},           /* / */
        {GLFW_KEY_0, Keyboard::Key::Alpha0},
        {GLFW_KEY_1, Keyboard::Key::Alpha1},
        {GLFW_KEY_2, Keyboard::Key::Alpha2},
        {GLFW_KEY_3, Keyboard::Key::Alpha3},
        {GLFW_KEY_4, Keyboard::Key::Alpha4},
        {GLFW_KEY_5, Keyboard::Key::Alpha5},
        {GLFW_KEY_6, Keyboard::Key::Alpha6},
        {GLFW_KEY_7, Keyboard::Key::Alpha7},
        {GLFW_KEY_8, Keyboard::Key::Alpha8},
        {GLFW_KEY_9, Keyboard::Key::Alpha9},
        {GLFW_KEY_SEMICOLON, Keyboard::Key::Semicolon}, /* ; */
        {GLFW_KEY_EQUAL, Keyboard::Key::Equal},         /* = */
        {GLFW_KEY_A, Keyboard::Key::A},
        {GLFW_KEY_B, Keyboard::Key::B},
        {GLFW_KEY_C, Keyboard::Key::C},
        {GLFW_KEY_D, Keyboard::Key::D},
        {GLFW_KEY_E, Keyboard::Key::E},
        {GLFW_KEY_F, Keyboard::Key::F},
        {GLFW_KEY_G, Keyboard::Key::G},
        {GLFW_KEY_H, Keyboard::Key::H},
        {GLFW_KEY_I, Keyboard::Key::I},
        {GLFW_KEY_J, Keyboard::Key::J},
        {GLFW_KEY_K, Keyboard::Key::K},
        {GLFW_KEY_L, Keyboard::Key::L},
        {GLFW_KEY_M, Keyboard::Key::M},
        {GLFW_KEY_N, Keyboard::Key::N},
        {GLFW_KEY_O, Keyboard::Key::O},
        {GLFW_KEY_P, Keyboard::Key::P},
        {GLFW_KEY_Q, Keyboard::Key::Q},
        {GLFW_KEY_R, Keyboard::Key::R},
        {GLFW_KEY_S, Keyboard::Key::S},
        {GLFW_KEY_T, Keyboard::Key::T},
        {GLFW_KEY_U, Keyboard::Key::U},
        {GLFW_KEY_V, Keyboard::Key::V},
        {GLFW_KEY_W, Keyboard::Key::W},
        {GLFW_KEY_X, Keyboard::Key::X},
        {GLFW_KEY_Y, Keyboard::Key::Y},
        {GLFW_KEY_Z, Keyboard::Key::Z},
        {GLFW_KEY_LEFT_BRACKET, Keyboard::Key::LeftBracket},   /* [ */
        {GLFW_KEY_BACKSLASH, Keyboard::Key::Backslash},        /* \ */
        {GLFW_KEY_RIGHT_BRACKET, Keyboard::Key::RightBracket}, /* ] */
        {GLFW_KEY_GRAVE_ACCENT, Keyboard::Key::GraveAccent},   /* ` */
        {GLFW_KEY_WORLD_1, Keyboard::Key::World1},             /* non-US #1 */
        {GLFW_KEY_WORLD_2, Keyboard::Key::World2},             /* non-US #2 */
        {GLFW_KEY_ESCAPE, Keyboard::Key::Escape},
        {GLFW_KEY_ENTER, Keyboard::Key::Enter},
        {GLFW_KEY_TAB, Keyboard::Key::Tab},
        {GLFW_KEY_BACKSPACE, Keyboard::Key::Backspace},
        {GLFW_KEY_INSERT, Keyboard::Key::Insert},
        {GLFW_KEY_DELETE, Keyboard::Key::Delete},
        {GLFW_KEY_RIGHT, Keyboard::Key::RightArrow},
        {GLFW_KEY_LEFT, Keyboard::Key::LeftArrow},
        {GLFW_KEY_DOWN, Keyboard::Key::DownArrow},
        {GLFW_KEY_UP, Keyboard::Key::KeyUp},
        {GLFW_KEY_PAGE_UP, Keyboard::Key::PageUp},
        {GLFW_KEY_PAGE_DOWN, Keyboard::Key::PageDown},
        {GLFW_KEY_HOME, Keyboard::Key::Home},
        {GLFW_KEY_END, Keyboard::Key::End},
        {GLFW_KEY_CAPS_LOCK, Keyboard::Key::CapsLock},
        {GLFW_KEY_SCROLL_LOCK, Keyboard::Key::ScrollLock},
        {GLFW_KEY_NUM_LOCK, Keyboard::Key::NumLock},
        {GLFW_KEY_PRINT_SCREEN, Keyboard::Key::PrintScreen},
        {GLFW_KEY_PAUSE, Keyboard::Key::Pause},
        {GLFW_KEY_F1, Keyboard::Key::F1},
        {GLFW_KEY_F2, Keyboard::Key::F2},
        {GLFW_KEY_F3, Keyboard::Key::F3},
        {GLFW_KEY_F4, Keyboard::Key::F4},
        {GLFW_KEY_F5, Keyboard::Key::F5},
        {GLFW_KEY_F6, Keyboard::Key::F6},
        {GLFW_KEY_F7, Keyboard::Key::F7},
        {GLFW_KEY_F8, Keyboard::Key::F8},
        {GLFW_KEY_F9, Keyboard::Key::F9},
        {GLFW_KEY_F10, Keyboard::Key::F10},
        {GLFW_KEY_F11, Keyboard::Key::F11},
        {GLFW_KEY_F12, Keyboard::Key::F12},
        {GLFW_KEY_F13, Keyboard::Key::F13},
        {GLFW_KEY_F14, Keyboard::Key::F14},
        {GLFW_KEY_F15, Keyboard::Key::F15},
        {GLFW_KEY_F16, Keyboard::Key::F16},
        {GLFW_KEY_F17, Keyboard::Key::F17},
        {GLFW_KEY_F18, Keyboard::Key::F18},
        {GLFW_KEY_F19, Keyboard::Key::F19},
        {GLFW_KEY_F20, Keyboard::Key::F20},
        {GLFW_KEY_F21, Keyboard::Key::F21},
        {GLFW_KEY_F22, Keyboard::Key::F22},
        {GLFW_KEY_F23, Keyboard::Key::F23},
        {GLFW_KEY_F24, Keyboard::Key::F24},
        {GLFW_KEY_F25, Keyboard::Key::F25},
        {GLFW_KEY_KP_0, Keyboard::Key::Keypad0},
        {GLFW_KEY_KP_1, Keyboard::Key::Keypad1},
        {GLFW_KEY_KP_2, Keyboard::Key::Keypad2},
        {GLFW_KEY_KP_3, Keyboard::Key::Keypad3},
        {GLFW_KEY_KP_4, Keyboard::Key::Keypad4},
        {GLFW_KEY_KP_5, Keyboard::Key::Keypad5},
        {GLFW_KEY_KP_6, Keyboard::Key::Keypad6},
        {GLFW_KEY_KP_7, Keyboard::Key::Keypad7},
        {GLFW_KEY_KP_8, Keyboard::Key::Keypad8},
        {GLFW_KEY_KP_9, Keyboard::Key::Keypad9},
        {GLFW_KEY_KP_DECIMAL, Keyboard::Key::KeypadDecimal},
        {GLFW_KEY_KP_DIVIDE, Keyboard::Key::KeypadDivide},
        {GLFW_KEY_KP_MULTIPLY, Keyboard::Key::KeypadMultiply},
        {GLFW_KEY_KP_SUBTRACT, Keyboard::Key::KeypadSubstract},
        {GLFW_KEY_KP_ADD, Keyboard::Key::KeypadAdd},
        {GLFW_KEY_KP_ENTER, Keyboard::Key::KeypadEnter},
        {GLFW_KEY_KP_EQUAL, Keyboard::Key::KeypadEqual},
        {GLFW_KEY_LEFT_SHIFT, Keyboard::Key::LeftShift},
        {GLFW_KEY_LEFT_CONTROL, Keyboard::Key::LeftControl},
        {GLFW_KEY_LEFT_ALT, Keyboard::Key::LeftAlt},
        {GLFW_KEY_LEFT_SUPER, Keyboard::Key::LeftSuper},
        {GLFW_KEY_RIGHT_SHIFT, Keyboard::Key::RightShift},
        {GLFW_KEY_RIGHT_CONTROL, Keyboard::Key::RightControl},
        {GLFW_KEY_RIGHT_ALT, Keyboard::Key::RightAlt},
        {GLFW_KEY_RIGHT_SUPER, Keyboard::Key::RightSuper},
        {GLFW_KEY_MENU, Keyboard::Key::Menu},
    };

    inline static HashMap<int, Mouse::Button> MouseButtonMapping = {
        {GLFW_MOUSE_BUTTON_LEFT, Mouse::Button::Left},
        {GLFW_MOUSE_BUTTON_RIGHT, Mouse::Button::Right},
        {GLFW_MOUSE_BUTTON_MIDDLE, Mouse::Button::Middle},
        {GLFW_MOUSE_BUTTON_4, Mouse::Button::Button4},
        {GLFW_MOUSE_BUTTON_5, Mouse::Button::Button5},
        {GLFW_MOUSE_BUTTON_6, Mouse::Button::Button6},
        {GLFW_MOUSE_BUTTON_7, Mouse::Button::Button7},
        {GLFW_MOUSE_BUTTON_8, Mouse::Button::Button8},
    };

    inline static HashMap<int, ButtonState> ButtonStateMapping = {
        {GLFW_PRESS, ButtonState::Pressed},
        {GLFW_RELEASE, ButtonState::Released},
    };

  public:
    void Initialize(InputService* pInputService)
    {
        assert(m_pInputService == nullptr && pInputService != nullptr);
        m_pInputService = pInputService;
    };

    void Shutdown()
    {
        m_pInputService = nullptr;
    }

    void MapKeyboardInput(int glfwKeyCode, int glfwAction)
    {
        // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
        if (glfwAction == GLFW_REPEAT)
        {
            return;
        }

        const auto& keyCode = KeyboardKeyMapping[glfwKeyCode];
        const auto& buttonState = ButtonStateMapping[glfwAction];

        // Update the control value
        m_pInputService->UpdateKeyboardControlState(keyCode, buttonState);
    }

    void MapMouseButtonInput(int glfwButtonCode, int glfwAction)
    {
        // Ignore GLFW key repeat events as they are unreliable. Eventually we should gather the events directly from the hardware.
        if (glfwAction == GLFW_REPEAT)
        {
            return;
        }

        auto& button = MouseButtonMapping[glfwButtonCode];
        auto& buttonState = ButtonStateMapping[glfwAction];

        // Update the control value
        m_pInputService->UpdateMouseControlState(button, buttonState);
    }
};

class GLFWApplication
{
  private:
    GLFWwindow* m_pGlfwWindow = nullptr;
    GLFWInputMapper m_glfwInputMapper;

    vkg::Instance m_instance; // Application-wide vulkan instance
    vk::UniqueSurfaceKHR m_pSurface;
    vkg::Device m_device;
    vkg::Swapchain m_swapchain;

    Engine m_engine;

  public:
    void Initialize()
    {
        // -------- Create GLFW Window
        glfwInit(); // Init glfw

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't use OpenGL context
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        // Grab monitor dimensions
        auto monitor = glfwGetPrimaryMonitor();
        int width, height;
        glfwGetMonitorWorkarea(monitor, nullptr, nullptr, &width, &height);

        // Create the GLFW window
        m_pGlfwWindow = glfwCreateWindow(width, height, "AllenEngine", nullptr, nullptr);

        // Adjust window dimensions and position to fit the screen, including title bars
        // Only frameTop is used on w10
        int frameLeft, frameRight, frameBottom, frameTop;
        glfwGetWindowFrameSize(m_pGlfwWindow, &frameLeft, &frameTop, &frameRight, &frameBottom);
        glfwSetWindowSize(m_pGlfwWindow, width, height - frameTop);
        glfwSetWindowPos(m_pGlfwWindow, 0, frameTop);
        glfwShowWindow(m_pGlfwWindow);
        glfwSetWindowUserPointer(m_pGlfwWindow, this);

        // Window callbacks
        glfwSetMouseButtonCallback(m_pGlfwWindow, MouseButtonCallback);
        glfwSetScrollCallback(m_pGlfwWindow, ScrollCallback);
        glfwSetKeyCallback(m_pGlfwWindow, KeyCallback);
        glfwSetFramebufferSizeCallback(m_pGlfwWindow, WindowResizeCallback);

        // --------- Create rendering resources
        // --- Vulkan instance
        uint32_t glfwExtensionCount;
        const char** glfwExtensions;

        // TODO: Use span
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // GLFW function that return the extensions it needs
        Vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        m_instance.Initialize(extensions);

        // --- Vulkan surface
        VkSurfaceKHR surface;
        auto res = glfwCreateWindowSurface(m_instance.GetVkInstance(), m_pGlfwWindow, nullptr, &surface);
        assert(res == VK_SUCCESS);

        m_pSurface = vk::UniqueSurfaceKHR(surface, m_instance.GetVkInstance());

        // --- Vulkan device
        m_device.Initialize(&m_instance, m_pSurface.get());

        // --- Vulkan swapchain
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(m_pGlfwWindow, &windowWidth, &windowHeight);

        m_swapchain.Initialize(&m_device, &m_pSurface.get(), windowWidth, windowHeight);

        // --------- Initialize engine
        m_engine.Initialize(m_pGlfwWindow, m_swapchain, m_device, {(float) windowWidth, (float) windowHeight});

        m_glfwInputMapper.Initialize(&m_engine.GetInputService());
    }

    void Shutdown()
    {
        m_device.GetVkDevice().waitIdle();

        m_glfwInputMapper.Shutdown();
        m_engine.Shutdown();

        glfwDestroyWindow(m_pGlfwWindow);
        glfwTerminate();
    }

    void Run()
    {
        while (!ShouldClose())
        {
            // Poll glfw events and forward them to the input service
            glfwPollEvents();

            double xpos, ypos;
            glfwGetCursorPos(m_pGlfwWindow, &xpos, &ypos);
            m_engine.GetInputService().UpdateMousePosition({(float) xpos, (float) ypos});

            // Gamepad input needs to be handled separately since glfw doesn't provide callbacks for them
            PollGamepadInput();

            int width, height;
            glfwGetFramebufferSize(m_pGlfwWindow, &width, &height);

            if (m_swapchain.RequiresResize())
            {
                m_swapchain.Resize(width, height);
            }

            bool windowMinimized = (width == 0 || height == 0);

            if (!windowMinimized)
            {
                m_engine.Update();
            }

            FrameMark;
        }
    };

    bool ShouldClose() const { return glfwWindowShouldClose(m_pGlfwWindow); }

  private:
    /// @brief Maps GLFW key events to the input system.
    /// TODO: override glfw and poll events directly from the os
    static void KeyCallback(GLFWwindow* pGlfwWindow, int key, int scancode, int action, int mods)
    {
        auto pApp = reinterpret_cast<GLFWApplication*>(glfwGetWindowUserPointer(pGlfwWindow));
        pApp->m_glfwInputMapper.MapKeyboardInput(key, action);
    }

    static void ScrollCallback(GLFWwindow* pGlfwWindow, double xoffset, double yoffset)
    {
        auto pApp = reinterpret_cast<GLFWApplication*>(glfwGetWindowUserPointer(pGlfwWindow));
        pApp->m_engine.GetInputService().UpdateScrollControlState(xoffset, yoffset);
    }

    static void MouseButtonCallback(GLFWwindow* pGlfwWindow, int button, int action, int mods)
    {
        auto pApp = reinterpret_cast<GLFWApplication*>(glfwGetWindowUserPointer(pGlfwWindow));
        pApp->m_glfwInputMapper.MapMouseButtonInput(button, action);
    }

    static void WindowResizeCallback(GLFWwindow* pGlfwWindow, int width, int height)
    {
        auto pApp = reinterpret_cast<GLFWApplication*>(glfwGetWindowUserPointer(pGlfwWindow));
        pApp->m_swapchain.TargetWindowResizedCallback(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    }

    void PollGamepadInput()
    {
        constexpr uint8_t MAX_JOYSTICKS = 16;
        for (auto joystickIdx = 0; joystickIdx < MAX_JOYSTICKS; ++joystickIdx)
        {
            if (glfwJoystickPresent(joystickIdx))
            {
                if (glfwJoystickIsGamepad(joystickIdx))
                {
                    // TODO: Handle multiple controllers
                    auto& gamepad = m_engine.GetInputService().m_gamepad;
                    GLFWgamepadstate state;

                    if (glfwGetGamepadState(joystickIdx, &state))
                    {
                        for (auto buttonIdx = 0; buttonIdx < GLFW_GAMEPAD_BUTTON_LAST; ++buttonIdx)
                        {
                            // Only trigger events if the state changed
                            if (state.buttons[buttonIdx] == GLFW_PRESS)
                            {
                                if (!gamepad.m_buttons[buttonIdx].IsHeld())
                                {
                                    // TODO: Map from GLFW button to our actual values
                                    gamepad.SetButtonPressed((Gamepad::Button) buttonIdx);
                                }
                            }
                            else
                            {
                                if (gamepad.m_buttons[buttonIdx].IsHeld())
                                {
                                    gamepad.SetButtonReleased((Gamepad::Button) buttonIdx);
                                }
                            }
                        }

                        gamepad.SetLeftStickState({state.axes[GLFW_GAMEPAD_AXIS_LEFT_X], state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]});
                        gamepad.SetRightStickState({state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]});
                        gamepad.SetLeftTriggerState(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
                        gamepad.SetRightTriggerState(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);
                    }
                }
            }
        }
    }
};
} // namespace aln

int main()
{
    aln::GLFWApplication* pApp = aln::New<aln::GLFWApplication>();

    pApp->Initialize();
    pApp->Run();
    pApp->Shutdown();

    aln::Delete(pApp); // Test deletion

#ifdef ALN_DEBUG
    tracy::GetProfiler().RequestShutdown();
    while (!tracy::GetProfiler().HasShutdownFinished())
    {
        continue;
    }
#endif

    return EXIT_SUCCESS;
}