#pragma once

#include "swapchain.hpp"

#include <common/maths/rectangle.hpp>
#include <common/maths/vec2.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{
class IWindow
{
  protected:
    vk::SurfaceKHR m_surface;
    Swapchain m_swapchain;

  public:
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;

    virtual void CreateSurface(const vk::Instance& instance) = 0;
    virtual void DestroySurface(const vk::Instance& instance) = 0;
    void CreateSwapchain(RenderEngine* pRenderEngine) { m_swapchain.Initialize(pRenderEngine, m_surface); }
    void DestroySwapchain(RenderEngine* pRenderEngine) { m_swapchain.Shutdown(pRenderEngine); }

    const vk::SurfaceKHR& GetSurface() const { return m_surface; }
    Swapchain& GetSwapchain() { return m_swapchain; }

    virtual Rectangle GetContentSize() const = 0;
    virtual Rectangle GetFrameSize() const = 0;
    virtual Rectangle GetFramebufferSize() const = 0;
    virtual bool ShouldClose() const = 0;
    virtual bool IsMinimized() const = 0;

    virtual Vec2 GetMousePosition() const = 0;

    virtual void* GetWindowPtr() const = 0;
};
} // namespace aln