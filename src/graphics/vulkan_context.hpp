#pragma once

#include "device.hpp"
#include "swapchain.hpp"
#include "window.hpp"

namespace vkg
{
/// @brief Vulkan rendering context.
class VulkanContext
{
    Window* m_pWindow;
    std::shared_ptr<Device> m_pDevice;
    Swapchain m_swapchain;

    ImGUI m_imgui;

    void Create(vkg::Window* pWindow)
    {
        assert(pWindow->IsInitialized());

        m_pWindow = pWindow;
        m_pDevice = std::make_shared<Device>(m_pWindow->GetSurface());
        m_swapchain = vkg::Swapchain(m_pDevice, &m_pWindow->GetSurface(), m_pWindow->GetWidth(), m_pWindow->GetHeight());

        // TODO: Pull Out imgui
        m_imgui.Initialize(m_pWindow->GetGLFWWindow(), m_pDevice, m_renderpass, m_targetSwapchain.NumberOfImages());
    };
}