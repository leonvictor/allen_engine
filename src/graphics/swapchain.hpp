#pragma once

#include "device.hpp"
#include "resources/image.hpp"
#include <vulkan/vulkan.hpp>
namespace vkg
{

/// @brief Wrapper around a vulkan swapchain. Swapchain represent an array of images we render to and that are presented to the screen.
class Swapchain
{
    friend class Renderer;

  public:
    Swapchain() {}

    Swapchain(std::shared_ptr<Device> pDevice, vk::SurfaceKHR* surface, uint32_t width, uint32_t height)
    {
        m_pDevice = pDevice;
        m_pSurface = surface;
        m_width = width;
        m_height = height;

        CreateInternal();
    }

    const vk::Format GetImageFormat() const { return m_surfaceFormat.format; }

    uint32_t AcquireNextImage(vk::Semaphore& semaphore)
    {
        auto result = m_pDevice->GetVkDevice().acquireNextImageKHR(m_vkSwapchain.get(), UINT64_MAX, semaphore, nullptr, &m_activeImageIndex);
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            Resize(m_width, m_height);
        }
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image.");
        }

        return m_activeImageIndex;
    }

    /// @brief Get the wrapped swapchain object.
    vk::SwapchainKHR& GetVkSwapchain() { return m_vkSwapchain.get(); }

    uint32_t GetWidth() const { return m_extent.width; }
    uint32_t GetHeight() const { return m_extent.height; }
    vk::Extent2D GetExtent() const { return m_extent; }
    std::shared_ptr<Device> GetDevice() const { return m_pDevice; }

    void Resize(uint32_t width, uint32_t height)
    {
        m_pDevice->GetVkDevice().waitIdle();
        if (width == 0 || height == 0)
        {
            throw std::runtime_error("Swapchain resize error");
        }

        m_width = width;
        m_height = height;

        CreateInternal();
    }

    void Present(vk::Semaphore& waitSemaphore)
    {
        // TODO: Pull out
        vk::PresentInfoKHR presentInfo;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_vkSwapchain.get();
        presentInfo.pImageIndices = &m_activeImageIndex;
        presentInfo.pResults = nullptr; // For checking every individual swap chain results. We only have one so we don't need it

        // TODO: Register swapchain renderers,
        // and add a callback to resize them
        // TODO: Rework cuz it's ugly (see https://github.com/liblava/liblava/blob/3bce924a014529a9d18cec9a406d3eab6850e159/liblava/frame/renderer.cpp)
        vk::Result result;
        try
        {
            result = m_pDevice->GetGraphicsQueue().GetVkQueue().presentKHR(presentInfo);
        }
        catch (vk::OutOfDateKHRError const& e)
        {
            result = vk::Result::eErrorOutOfDateKHR;
        }

        // TODO: Shoud this happen in swapchain directly ?
        // TODO: Put back resize !
        // if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_pWindow->m_framebufferResized)
        // {
        //     // TODO: SHouldn't be here
        //     while (m_pWindow->IsMinimized())
        //     {
        //         m_pWindow->WaitEvents();
        //     }

        //     auto size = m_pWindow->GetSize();
        //     Resize(size.width, size.height);

        //     // TODO: Plug callbacks for associated renderpass and pipelines (in renderer)
        //     // m_renderpass.Resize(&m_targetSwapchain);
        //     // CreatePipelines();

        //     m_pWindow->m_framebufferResized = false;
        // }
        // else if (result != vk::Result::eSuccess)
        // {
        //     throw std::runtime_error("Failed to present swap chain image.");
        // }
    }

  private:
    // Wrapped vulkan swapchain.
    vk::UniqueSwapchainKHR m_vkSwapchain;

    vk::SurfaceKHR* m_pSurface = nullptr;
    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::Extent2D m_extent;

    std::shared_ptr<Device> m_pDevice;
    uint32_t m_width, m_height;
    uint32_t m_activeImageIndex;

    void CreateInternal()
    {
        // Create the swapchain; reusing the info if it has already been created
        // TODO: make sure oldSwapchain is destroyed with RAII
        vk::UniqueSwapchainKHR oldSwapchain = std::move(m_vkSwapchain);

        auto createInfo = CreateInfo(&oldSwapchain.get());
        m_vkSwapchain = m_pDevice->GetVkDevice().createSwapchainKHRUnique(createInfo);
    }

    /// @brief Generate the vulkan CreateInfo struct for a swapchain object.
    vk::SwapchainCreateInfoKHR CreateInfo(vk::SwapchainKHR* pOldSwapchain = nullptr)
    {
        assert(m_pSurface != nullptr);

        auto swapchainSupport = m_pDevice->GetSwapchainSupport(*m_pSurface);
        auto presentMode = ChoosePresentMode(swapchainSupport.presentModes);
        m_surfaceFormat = ChooseSurfaceFormat(swapchainSupport.formats); // Defaults to B8G8R8A8Srgb

        // TODO: Reuse current extent if an old swapchain is provided
        // TODO: width, height ? -> maybe grab from a vkg::window ?
        m_extent = ChooseExtent(swapchainSupport.capabilities, m_width, m_height);

        uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
        if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
        {
            imageCount = swapchainSupport.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo{
            .surface = *m_pSurface,
            .minImageCount = imageCount,
            .imageFormat = m_surfaceFormat.format,
            .imageColorSpace = m_surfaceFormat.colorSpace,
            .imageExtent = m_extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .preTransform = swapchainSupport.capabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = presentMode,
            .clipped = VK_TRUE, // We don't care about the color of obscured pixels (ex: if another window is on top)
            .oldSwapchain = *pOldSwapchain,
        };

        uint32_t queueFamilyIndices[] = {
            m_pDevice->GetGraphicsQueue().GetFamilyIndex(),
            m_pDevice->GetPresentQueue().GetFamilyIndex(),
        };

        if (queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
            createInfo.queueFamilyIndexCount = 0;     // Optionnal
            createInfo.pQueueFamilyIndices = nullptr; // Optionnal
        }

        return createInfo;
    }

    vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            // Some window managers do not specify the resolution (indicated by special max value)
            // In this case, use the resolution that best matches the window within the ImageExtent bounds
            vk::Extent2D extent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height),
            };

            extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            return extent;
        }
    }

    vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats, const vk::Format& desiredFormat = vk::Format::eB8G8R8A8Srgb)
    {
        // TODO: Refine selection (ex: support bliting to linear tiling format)
        for (const auto& format : availableFormats)
        {
            // Return the first format that supports SRGB color space in 8bit by channels
            // TODO: We might want to switch == to & if we have multiple acceptable formats
            if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && format.format == desiredFormat)
            {
                return format;
            }
        }
        return availableFormats[0];
    }

    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes)
    {
        for (const auto& mode : availableModes)
        {
            if (mode == vk::PresentModeKHR::eMailbox)
            {
                return mode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }
};
} // namespace vkg