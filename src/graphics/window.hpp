#pragma once

#include "instance.hpp"
#include <GLFW/glfw3.h>
#include <assert.h>

namespace vkg
{

struct Size2D
{
    int width;
    int height;
};

// TODO: Where do default sizes go ?
// Default width
const int WIDTH = 800;
// Default height
const int HEIGHT = 600;

/// @brief Represent an on-screen window. Holds the OS window and the related vulkan objects.
/// Also wraps the window library (GLFW).
class Window
{

  public:
    ~Window()
    {
        glfwDestroyWindow(m_pGlfwWindow);
        glfwTerminate();
    }

    // TODO: Temporary. Make private when possible
    bool m_framebufferResized = false;

    void Initialize()
    {
        InitializeWindow();
        // TODO: This is not that cool
        Instance::Singleton().Create();
        CreateSurface();
        m_status = State::Initialized;
    }

    inline bool IsInitialized() const { return m_status == State::Initialized; }

    /// @brief Return the current size of the display window.
    Size2D GetSize() const
    {
        Size2D size;
        // TODO: Cache size, GetWidth(), GetHeight()
        glfwGetFramebufferSize(m_pGlfwWindow, &size.width, &size.height);
        return size;
    }

    uint32_t GetWidth() const { return (uint32_t) GetSize().width; }
    uint32_t GetHeight() const { return (uint32_t) GetSize().height; }

    float GetAspectRatio() const
    {
        auto size = GetSize();
        if (size.height == 0)
            return 0.0;
        return size.width / size.height;
    }

    bool IsMinimized()
    {
        auto size = GetSize();
        return (size.width == 0 || size.height == 0);
    }

    void WaitEvents() { glfwWaitEvents(); }

    vk::SurfaceKHR& GetSurface()
    {
        return m_vkSurface.get();
    }

    GLFWwindow* GetGLFWWindow()
    {
        return m_pGlfwWindow;
    }

  private:
    enum State
    {
        Uninitialized,
        WindowReady,
        Initialized
    };

    // TODO: Temporary. Make private when possible
    GLFWwindow* m_pGlfwWindow;

    State m_status = State::Uninitialized;
    vk::UniqueSurfaceKHR m_vkSurface;

    // TODO: find a good way to handle extensions. Maybe populating a list in the singleton instance ?
    // Right now things are weird because we have to split the initialization in two
    // Glfw must be initialized to get the required extension, then instance needs to be created to build the surface
    std::vector<const char*> GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // GLFW function that return the extensions it needs
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        return extensions;
    }

    static void FramebufferResizeCallback(GLFWwindow* pGlfwWindow, int width, int height)
    {
        auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGlfwWindow));
        window->m_framebufferResized = true;
    }

    // TODO: Use InputMonitor.
    // FIXME: GLFW events for keyboard and mouse might share the same identifiers
    static void KeyCallback(GLFWwindow* pGlfwWindow, int key, int scancode, int action, int mods)
    {
        auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGlfwWindow));

        if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
        {
            // window->objectsToCreate++;
        }
        else if (key == GLFW_KEY_X && action == GLFW_RELEASE)
        {
            // auto index = window->models.size() - 1;
            // window->objectToDelete = index;
        }
    }

    static void ScrollCallback(GLFWwindow* pGlfwWindow, double xoffset, double yoffset)
    {
        auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGlfwWindow));
        // // TODO: where do cameras go ?
        // window->camera.zoom(yoffset);
    }

    static void MouseButtonCallback(GLFWwindow* pGlfwWindow, int button, int action, int mods)
    {
        auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGlfwWindow));

        //TODO: This doesn't need to happen for every frame
        double xpos, ypos;
        glfwGetCursorPos(pGlfwWindow, &xpos, &ypos);
        // TODO: Where do inputs go ?
        // window->lastMousePos = {xpos, ypos};
        // window->input.callback(button, action, mods);
    }

    /// @brief Initialize the OS window (here GLFW)
    /// TODO: Find a better name, this is confusing
    void InitializeWindow()
    {
        glfwInit();                                   // Init glfw
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't use OpenGL context

        // if (glfwRawMouseMotionSupported()) {
        //     glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        // }

        m_pGlfwWindow = glfwCreateWindow(WIDTH, HEIGHT, "PoopyEngine", nullptr, nullptr);
        glfwSetWindowUserPointer(m_pGlfwWindow, this);

        // TODO: where do the callbacks live ?
        // glfwSetMouseButtonCallback(m_pGlfwWindow, mouseButtonCallback);
        // glfwSetScrollCallback(m_pGlfwWindow, scrollCallback);
        glfwSetFramebufferSizeCallback(m_pGlfwWindow, FramebufferResizeCallback);
        // glfwSetKeyCallback(m_pGlfwWindow, keyCallback);

        Instance::Singleton().RequestExtensions(GetRequiredExtensions());
        m_status = State::WindowReady;
    }

    /// @brief Create the vulkan surface
    void CreateSurface()
    {
        assert(m_status == State::WindowReady);
        assert(Instance::Singleton().IsInitialized()), "Tried to create the surface before the instance.";

        VkSurfaceKHR pSurface;
        auto res = glfwCreateWindowSurface((VkInstance) Instance::Singleton().Get(), m_pGlfwWindow, nullptr, &pSurface);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface.");
        }

        m_vkSurface = vk::UniqueSurfaceKHR(pSurface, Instance::Singleton().Get());
    }
};
} // namespace vkg