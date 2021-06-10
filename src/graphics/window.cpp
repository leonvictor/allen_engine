#include "window.hpp"

#include "../input/input_system.hpp"
#include "instance.hpp"

#include <assert.h>

namespace vkg
{

Window::~Window()
{
    glfwDestroyWindow(m_pGlfwWindow);
    glfwTerminate();
}

void Window::Initialize()
{
    InitializeWindow();
    // TODO: This is not that cool
    Instance::Create();
    CreateSurface();
    m_status = State::Initialized;
}

Size2D Window::GetSize() const
{
    Size2D size;
    // TODO: Cache size, GetWidth(), GetHeight()
    glfwGetFramebufferSize(m_pGlfwWindow, &size.width, &size.height);
    return size;
}

float Window::GetAspectRatio() const
{
    auto size = GetSize();
    if (size.height == 0)
        return 0.0;
    return size.width / size.height;
}

bool Window::IsMinimized()
{
    auto size = GetSize();
    return (size.width == 0 || size.height == 0);
}

std::vector<const char*> Window::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // GLFW function that return the extensions it needs
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return extensions;
}

void Window::FramebufferResizeCallback(GLFWwindow* pGlfwWindow, int width, int height)
{
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGlfwWindow));
    window->m_framebufferResized = true;
}

void Window::KeyCallback(GLFWwindow* pGlfwWindow, int key, int scancode, int action, int mods)
{
    // FIXME: GLFW events for keyboard and mouse might share the same identifiers ?
    Input::Keyboard.UpdateControlState(scancode, action);
}

void Window::ScrollCallback(GLFWwindow* pGlfwWindow, double xoffset, double yoffset)
{
    Input::Mouse.UpdateScrollControlState(xoffset, yoffset);
}

void Window::MouseButtonCallback(GLFWwindow* pGlfwWindow, int button, int action, int mods)
{
    Input::Mouse.UpdateControlState(button, action);
}

void Window::InitializeWindow()
{
    glfwInit(); // Init glfw

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't use OpenGL context
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // Grab monitor dimensions
    auto monitor = glfwGetPrimaryMonitor();
    int width, height;
    glfwGetMonitorWorkarea(monitor, nullptr, nullptr, &width, &height);

    // Create the GLFW window
    m_pGlfwWindow = glfwCreateWindow(width, height, "PoopyEngine", nullptr, nullptr);

    // Adjust window dimensions and position to fit the screen, including title bars
    // Only frameTop is used on w10
    int frameLeft, frameRight, frameBottom, frameTop;
    glfwGetWindowFrameSize(m_pGlfwWindow, &frameLeft, &frameTop, &frameRight, &frameBottom);
    glfwSetWindowSize(m_pGlfwWindow, width, height - frameTop);
    glfwSetWindowPos(m_pGlfwWindow, 0, frameTop);
    glfwShowWindow(m_pGlfwWindow);

    glfwSetWindowUserPointer(m_pGlfwWindow, this);

    // Callbacks
    glfwSetMouseButtonCallback(m_pGlfwWindow, MouseButtonCallback);
    glfwSetScrollCallback(m_pGlfwWindow, ScrollCallback);
    glfwSetKeyCallback(m_pGlfwWindow, KeyCallback);
    glfwSetFramebufferSizeCallback(m_pGlfwWindow, FramebufferResizeCallback);

    Instance::RequestExtensions(GetRequiredExtensions());
    m_status = State::WindowReady;
}

void Window::CreateSurface()
{
    assert(m_status == State::WindowReady);
    assert(Instance::IsInitialized()), "Tried to create the surface before the instance.";

    VkSurfaceKHR pSurface;
    auto res = glfwCreateWindowSurface((VkInstance) Instance::Get(), m_pGlfwWindow, nullptr, &pSurface);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface.");
    }

    m_vkSurface = vk::UniqueSurfaceKHR(pSurface, Instance::Get());
}
} // namespace vkg