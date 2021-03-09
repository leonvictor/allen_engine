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

class Window
{

  public:
    // TODO: Where do default sizes go ?

    // TODO: Temporary. Make private when possible
    GLFWwindow* m_pGlfwWindow;

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
        // glfwSetFramebufferSizeCallback(m_pGlfwWindow, framebufferResizeCallback);
        // glfwSetKeyCallback(m_pGlfwWindow, keyCallback);

        core::Instance::Singleton().RequestExtensions(GetRequiredExtensions());
        m_status = State::WindowReady;
    }

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

    Size2D GetSize()
    {
        Size2D size;
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
    uint8_t m_width, m_height;
    vk::UniqueSurfaceKHR m_vkSurface;

    // TODO: find a good way to handle extensions. Maybe populating a list in the singleton instance ?
    // Right things are weird because we have to split the initialization in two
    // Glfw must be initialized to get the required extension, then instance needs to be created to build the surface
    std::vector<const char*> GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // GLFW function that return the extensions it needs
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        return extensions;
    }
};
} // namespace vkg