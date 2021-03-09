#pragma once

#include "../core/instance.hpp"
#include <GLFW/glfw3.h>
#include <assert.h>

namespace vkg
{

struct Size2D
{
    int width;
    int height;
};

const int WIDTH = 800;
const int HEIGHT = 600;

/// @brief Represent an on-screen window. Holds the OS window and the related vulkan objects.
/// Also wraps the window library (GLFW).
class Window
{

  public:
    // TODO: Where do default sizes go ?

    // TODO: Temporary. Make private when possible
    GLFWwindow* m_pGlfwWindow;
    // TODO: Temporary. Make private when possible
    bool m_framebufferResized = false;

    void Initialize()
    {
        InitializeWindow();
        // TODO: This is not that cool
        core::Instance::Singleton().Create();
        CreateSurface();
    }

    /// @brief Return the current size of the display window.
    Size2D GetSize()
    {
        Size2D size;
        // TODO: Cache size, GetWidth(), GetHeight()
        glfwGetFramebufferSize(m_pGlfwWindow, &size.width, &size.height);
        return size;
    }

    vk::UniqueSurfaceKHR& GetSurface()
    {
        return m_vkSurface;
    }

  private:
    enum State
    {
        Uninitialized,
        WindowReady,
        Initinalized
    };

    State m_status;
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

        // TODO: where do the callbacks live ?
        // glfwSetWindowUserPointer(m_pGlfwWindow, this);
        // glfwSetMouseButtonCallback(m_pGlfwWindow, mouseButtonCallback);
        // glfwSetScrollCallback(m_pGlfwWindow, scrollCallback);
        glfwSetFramebufferSizeCallback(m_pGlfwWindow, FramebufferResizeCallback);
        // glfwSetKeyCallback(m_pGlfwWindow, keyCallback);

        core::Instance::Singleton().RequestExtensions(GetRequiredExtensions());
        m_status = State::WindowReady;
    }

    /// @brief Create the vulkan surface
    void CreateSurface()
    {
        assert(m_status == State::WindowReady);
        assert(core::Instance::Singleton().IsInitialized()), "Tried to create the surface before the instance.";

        VkSurfaceKHR pSurface;
        auto res = glfwCreateWindowSurface((VkInstance) core::Instance::Singleton().Get(), m_pGlfwWindow, nullptr, &pSurface);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface.");
        }

        m_vkSurface = vk::UniqueSurfaceKHR(pSurface, core::Instance::Singleton().Get());
    }
};
} // namespace vkg