#pragma once

#include "engine.hpp"
#include "glfw_window.hpp"

#include <common/memory.hpp>
#include <graphics/instance.hpp>
#include <graphics/render_engine.hpp>
#include <graphics/swapchain.hpp>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace aln
{

class GLFWApplication
{
  private:
    GLFWInputMapper m_glfwInputMapper;
    GlfwWindow m_window;

    vk::UniqueSurfaceKHR m_pSurface;

    Engine m_engine;

  public:
    void Initialize()
    {
        m_window.Initialize();
        m_engine.Initialize(&m_window);

        m_window.SetMouseButtonCallback([&](Mouse::Button button, ButtonState state)
            { m_engine.GetInputService().UpdateMouseControlState(button, state); });
        m_window.SetKeyCallback([&](Keyboard::Key key, ButtonState state)
            { m_engine.GetInputService().UpdateKeyboardControlState(key, state); });
        m_window.SetScrollCallback([&](float x, float y)
            { m_engine.GetInputService().UpdateScrollControlState(x, y); });
        m_window.SetSFramebufferResizedCallback([&](uint32_t width, uint32_t height)
            { m_engine.m_renderEngine.GetWindow()->GetSwapchain().TargetWindowResizedCallback(width, height); });
    }

    void Shutdown()
    {
        m_engine.Shutdown();
        m_window.Shutdown();
    }

    void Run()
    {
        while (!ShouldClose())
        {
            // TODO: Move to glfw window class
            // Poll glfw events and forward them to the input service
            glfwPollEvents();

            m_engine.GetInputService().UpdateMousePosition(m_window.GetMousePosition());

            // Gamepad input needs to be handled separately since glfw doesn't provide callbacks for them
            PollGamepadInput();

            auto framebufferSize = m_window.GetFramebufferSize();
            if (m_engine.m_renderEngine.GetWindow()->GetSwapchain().RequiresResize())
            {
                m_engine.m_renderEngine.GetWindow()->GetSwapchain().Resize(framebufferSize.width, framebufferSize.height);
            }

            if (!m_window.IsMinimized())
            {
                m_engine.Update();
            }

            FrameMark;
        }
    };

    bool ShouldClose() const { return m_window.ShouldClose(); }

  private:
    void PollGamepadInput()
    {
        constexpr uint8_t MAX_JOYSTICKS = 16;
        //  TMP: Only use 1 joystick
        // for (auto joystickIdx = 0; joystickIdx < MAX_JOYSTICKS; ++joystickIdx)
        //{
        auto joystickIdx = 0;
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
        //}
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