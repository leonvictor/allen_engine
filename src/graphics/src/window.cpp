#include "window.hpp"
#include "instance.hpp"

#include <assert.h>

namespace vkg
{

Window::~Window()
{
    glfwDestroyWindow(m_pGlfwWindow);
    glfwTerminate();
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

void Window::AddResizeCallback(std::function<void(int, int)> callback)
{
    m_resizeCallbacks.push_back(callback);
}

void Window::AddScrollCallback(std::function<void(int, int)> callback)
{
    m_scrollCallbacks.push_back(callback);
}

void Window::AddKeyCallback(std::function<void(int, int)> callback)
{
    m_keyCallbacks.push_back(callback);
}

void Window::AddMouseButtonCallback(std::function<void(int, int)> callback)
{
    m_mouseButtonCallbacks.push_back(callback);
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
    for (auto& callback : window->m_resizeCallbacks)
    {
        callback(width, height);
    }
}

void Window::KeyCallback(GLFWwindow* pGlfwWindow, int key, int scancode, int action, int mods)
{
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGlfwWindow));
    for (auto& callback : window->m_keyCallbacks)
    {
        callback(scancode, action);
    }
    // FIXME: GLFW events for keyboard and mouse might share the same identifiers ?
    // Input::UpdateKeyboardControlState(scancode, action);
}

void Window::ScrollCallback(GLFWwindow* pGlfwWindow, double xoffset, double yoffset)
{
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGlfwWindow));
    for (auto& callback : window->m_scrollCallbacks)
    {
        callback(xoffset, yoffset);
    }
    // Input::UpdateScrollControlState(xoffset, yoffset);
}

void Window::MouseButtonCallback(GLFWwindow* pGlfwWindow, int button, int action, int mods)
{
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGlfwWindow));
    for (auto& callback : window->m_mouseButtonCallbacks)
    {
        callback(button, action);
    }
    // Input::UpdateMouseControlState(button, action);
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

    // Instance::RequestExtensions(GetRequiredExtensions());
    m_status = State::WindowReady;
}

void Window::CreateSurface(const vkg::Instance* pInstance)
{
    assert(m_status == State::WindowReady);
    assert(pInstance->IsInitialized());

    VkSurfaceKHR pSurface;
    auto res = glfwCreateWindowSurface((VkInstance) pInstance->GetVkInstance(), m_pGlfwWindow, nullptr, &pSurface);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface.");
    }

    m_vkSurface = vk::UniqueSurfaceKHR(pSurface, pInstance->GetVkInstance());
    m_status = State::Initialized;
}

void Window::NewFrame()
{

    glfwPollEvents();
}

glm::vec2 Window::GetCursorPosition() const
{
    double xpos, ypos;
    glfwGetCursorPos(m_pGlfwWindow, &xpos, &ypos);
    return {xpos, ypos};
}

} // namespace vkg