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
class GLFWApplication
{
  private:
    GLFWwindow* m_pGlfwWindow = nullptr;

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
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

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
        m_engine.Initialize(m_pGlfwWindow, m_swapchain, m_device, {windowWidth, windowHeight});
    }

    void Shutdown()
    {
        m_device.GetVkDevice().waitIdle();
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
            m_engine.GetInputService().UpdateMousePosition({xpos, ypos});

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
        // TODO: Handle glfw mappings here rather than in the keyboard class
        auto pApp = reinterpret_cast<GLFWApplication*>(glfwGetWindowUserPointer(pGlfwWindow));
        pApp->m_engine.GetInputService().UpdateKeyboardControlState(key, scancode);
    }

    static void ScrollCallback(GLFWwindow* pGlfwWindow, double xoffset, double yoffset)
    {
        auto pApp = reinterpret_cast<GLFWApplication*>(glfwGetWindowUserPointer(pGlfwWindow));
        pApp->m_engine.GetInputService().UpdateScrollControlState(xoffset, yoffset);
    }

    static void MouseButtonCallback(GLFWwindow* pGlfwWindow, int button, int action, int mods)
    {
        auto pApp = reinterpret_cast<GLFWApplication*>(glfwGetWindowUserPointer(pGlfwWindow));
        pApp->m_engine.GetInputService().UpdateMouseControlState(button, action);
    }

    static void WindowResizeCallback(GLFWwindow* pGlfwWindow, int width, int height)
    {
        auto pApp = reinterpret_cast<GLFWApplication*>(glfwGetWindowUserPointer(pGlfwWindow));
        pApp->m_swapchain.TargetWindowResizedCallback(width, height);
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
#ifdef ALN_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    aln::GLFWApplication* pApp = aln::New<aln::GLFWApplication>();

    pApp->Initialize();
    pApp->Run();
    pApp->Shutdown();

    aln::Delete(pApp); // Test deletion

    return EXIT_SUCCESS;
}