#pragma once

#include "resources/image.hpp"

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

#include <functional>

namespace aln
{
class GLFWApplication;
}
namespace aln
{

class RenderEngine;
namespace resources
{
class Image;
}

/// @brief Wrapper around a vulkan swapchain. Swapchain represent an array of images we render to and that are presented to the screen.
class Swapchain
{
    friend class Renderer;
    friend GLFWApplication;

  private:
    RenderEngine* m_pRenderEngine;
    // TODO: Get rid of the reference to the associated surface. For now we need to have it cached for swapchain recreation
    vk::SurfaceKHR* m_pSurface = nullptr;

    // Wrapped vulkan swapchain.
    vk::SwapchainKHR m_swapchain;
    Vector<vk::Image> m_images;
    Vector<vk::Semaphore> m_imageAvailableSemaphores;

    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::Extent2D m_extent;

    uint32_t m_width, m_height;
    uint32_t m_activeImageIndex;
    bool m_resizeRequired = false;

    Vector<std::function<void(uint32_t, uint32_t)>> m_resizeCallbacks;

  private:
    void CreateImageViews();
    void CreateInternal();

    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
    vk::SurfaceFormatKHR ChooseSurfaceFormat(const Vector<vk::SurfaceFormatKHR>& availableFormats, const vk::Format& desiredFormat = vk::Format::eB8G8R8A8Srgb);
    vk::PresentModeKHR ChoosePresentMode(const Vector<vk::PresentModeKHR>& availableModes);

    void TargetWindowResizedCallback(uint32_t width, uint32_t height);

  public:
    void Initialize(RenderEngine* pRenderEngine, vk::SurfaceKHR& surface);
    void Shutdown(RenderEngine* pRenderEngine);

    uint32_t AcquireNextImage();
    vk::Semaphore& GetFrameImageAvailableSemaphore();

    /// @brief Get the wrapped swapchain object.
    inline vk::SwapchainKHR& GetVkSwapchain() { return m_swapchain; }

    inline const vk::Format GetImageFormat() const { return m_surfaceFormat.format; }
    inline uint32_t GetWidth() const { return m_extent.width; }
    inline uint32_t GetHeight() const { return m_extent.height; }
    inline vk::Extent2D GetExtent() const { return m_extent; }
    inline RenderEngine* GetDevice() const { return m_pRenderEngine; }
    uint32_t GetImageCount() const { return m_images.size(); }
    vk::Image& GetImage(uint32_t imageIdx) { return m_images[imageIdx]; }
    uint32_t GetCurrentImageIdx() const { return m_activeImageIndex; }

    /// @brief Recreate the swapchain with the desired size. Will also trigger registered callbacks.
    void Resize(uint32_t width, uint32_t height);

    void Present(vk::Semaphore& waitSemaphore);

    /// @brief Add a callback that triggers when this swapchain is resized.
    void AddResizeCallback(std::function<void(uint32_t, uint32_t)> callback);
    bool RequiresResize() const { return m_resizeRequired; }
};
} // namespace aln