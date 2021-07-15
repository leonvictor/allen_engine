#pragma once

#include <vulkan/vulkan.hpp>

namespace aln::vkg
{

class Device;
class Image;
class Window;

/// @brief Wrapper around a vulkan swapchain. Swapchain represent an array of images we render to and that are presented to the screen.
class Swapchain
{
    friend class Renderer;

  public:
    Swapchain() {}

    Swapchain(std::shared_ptr<Device> pDevice, vkg::Window* pWindow);

    uint32_t AcquireNextImage(vk::Semaphore& semaphore);

    /// @brief Get the wrapped swapchain object.
    inline vk::SwapchainKHR& GetVkSwapchain() { return m_vkSwapchain.get(); }

    inline const vk::Format GetImageFormat() const { return m_surfaceFormat.format; }
    inline uint32_t GetWidth() const { return m_extent.width; }
    inline uint32_t GetHeight() const { return m_extent.height; }
    inline vk::Extent2D GetExtent() const { return m_extent; }
    inline std::shared_ptr<Device> GetDevice() const { return m_pDevice; }

    /// @brief Recreate the swapchain with the desired size. Will also trigger registered callbacks.
    void Resize(uint32_t width, uint32_t height);

    void Present(vk::Semaphore& waitSemaphore);

    /// @brief Add a callback that triggers when this swapchain is resized.
    void AddResizeCallback(std::function<void(uint32_t, uint32_t)> callback);

  private:
    // Wrapped vulkan swapchain.
    vk::UniqueSwapchainKHR m_vkSwapchain;

    vkg::Window* m_pWindow = nullptr;
    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::Extent2D m_extent;

    std::shared_ptr<Device> m_pDevice;
    uint32_t m_width, m_height;
    uint32_t m_activeImageIndex;
    bool m_resizeRequired = false;

    std::vector<std::function<void(uint32_t, uint32_t)>> m_resizeCallbacks;

    void CreateInternal();

    void TargetWindowResizedCallback(int width, int height);

    /// @brief Generate the vulkan CreateInfo struct for a swapchain object.
    vk::SwapchainCreateInfoKHR CreateInfo(vk::SwapchainKHR* pOldSwapchain = nullptr);

    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats, const vk::Format& desiredFormat = vk::Format::eB8G8R8A8Srgb);

    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes);
};
} // namespace vkg