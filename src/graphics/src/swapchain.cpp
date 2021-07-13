#include "swapchain.hpp"

#include "device.hpp"
#include "resources/image.hpp"
#include "window.hpp"

#include <functional>

namespace aln::vkg
{

Swapchain::Swapchain(std::shared_ptr<Device> pDevice, vkg::Window* pWindow)
{
    m_pDevice = pDevice;
    m_pWindow = pWindow;
    m_width = pWindow->GetWidth();
    m_height = pWindow->GetHeight();

    // Hook a callback to the window's resize event
    pWindow->AddResizeCallback(std::bind(&Swapchain::TargetWindowResizedCallback, this, std::placeholders::_1, std::placeholders::_2));

    CreateInternal();
}

void Swapchain::TargetWindowResizedCallback(int width, int height)
{
    m_resizeRequired = true;
}

uint32_t Swapchain::AcquireNextImage(vk::Semaphore& semaphore)
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

void Swapchain::Resize(uint32_t width, uint32_t height)
{
    m_pDevice->GetVkDevice().waitIdle();
    if (width == 0 || height == 0)
    {
        throw std::runtime_error("Swapchain resize error");
    }

    m_width = width;
    m_height = height;

    CreateInternal();

    // Trigger callbacks
    for (auto callback : m_resizeCallbacks)
    {
        callback(m_width, m_height);
    }

    m_resizeRequired = false;
}

void Swapchain::Present(vk::Semaphore& waitSemaphore)
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
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_resizeRequired)
    {
        // TODO: Shouldn't be here
        while (m_pWindow->IsMinimized())
        {
            m_pWindow->WaitEvents();
        }

        auto size = m_pWindow->GetSize();
        Resize(size.width, size.height);
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image.");
    }
}

void Swapchain::CreateInternal()
{
    // Create the swapchain; reusing the info if it has already been created
    // TODO: make sure oldSwapchain is destroyed with RAII
    vk::UniqueSwapchainKHR oldSwapchain = std::move(m_vkSwapchain);

    auto createInfo = CreateInfo(&oldSwapchain.get());
    m_vkSwapchain = m_pDevice->GetVkDevice().createSwapchainKHRUnique(createInfo);
}

vk::SwapchainCreateInfoKHR Swapchain::CreateInfo(vk::SwapchainKHR* pOldSwapchain)
{
    auto swapchainSupport = m_pDevice->GetSwapchainSupport(m_pWindow->GetVkSurface());
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
        .surface = m_pWindow->GetVkSurface(),
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

vk::Extent2D Swapchain::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
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

vk::SurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats, const vk::Format& desiredFormat)
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

vk::PresentModeKHR Swapchain::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes)
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

void Swapchain::AddResizeCallback(std::function<void(uint32_t, uint32_t)> callback)
{
    m_resizeCallbacks.push_back(callback);
}

} // namespace aln::vkg