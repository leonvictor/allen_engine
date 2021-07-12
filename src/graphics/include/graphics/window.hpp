#pragma once

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <functional>
#include <vector>

namespace vkg
{

class Instance;

struct Size2D
{
    int width;
    int height;
};

/// @brief Represent an on-screen window. Holds the OS window and the related vulkan objects.
/// Also wraps the window library (GLFW).
class Window
{
    friend class Engine;

  public:
    ~Window();

    inline bool IsInitialized() const { return m_status == State::Initialized; }

    /// @brief Return the current size of the display window.
    Size2D GetSize() const;

    inline uint32_t GetWidth() const { return (uint32_t) GetSize().width; }
    inline uint32_t GetHeight() const { return (uint32_t) GetSize().height; }

    float GetAspectRatio() const;

    bool IsMinimized();

    void WaitEvents() { glfwWaitEvents(); }

    inline vk::SurfaceKHR& GetVkSurface() { return m_vkSurface.get(); }
    inline GLFWwindow* GetGLFWWindow() { return m_pGlfwWindow; }

    void AddResizeCallback(std::function<void(int, int)> callback);

    void NewFrame();

  private:
    enum class State
    {
        Uninitialized,
        WindowReady,
        Initialized
    };

    GLFWwindow* m_pGlfwWindow;

    State m_status = State::Uninitialized;
    vk::UniqueSurfaceKHR m_vkSurface;
    std::vector<std::function<void(uint32_t, uint32_t)>> m_resizeCallbacks;

    // TODO: find a good way to handle extensions. Maybe populating a list in the singleton instance ?
    // Right now things are weird because we have to split the initialization in two
    // Glfw must be initialized to get the required extension, then instance needs to be created to build the surface
    std::vector<const char*> GetRequiredExtensions();

    static void FramebufferResizeCallback(GLFWwindow* pGlfwWindow, int width, int height);

    /// @brief Maps GLFW key events to the input system.
    /// TODO: override glfw and poll events directly from the os
    static void KeyCallback(GLFWwindow* pGlfwWindow, int key, int scancode, int action, int mods);

    static void ScrollCallback(GLFWwindow* pGlfwWindow, double xoffset, double yoffset);

    static void MouseButtonCallback(GLFWwindow* pGlfwWindow, int button, int action, int mods);

    /// @brief Initialize the OS window (here GLFW)
    /// TODO: Find a better name, this is confusing
    void InitializeWindow();

    /// @brief Create the vulkan surface
    void CreateSurface(const vkg::Instance* pInstance);
};
} // namespace vkg