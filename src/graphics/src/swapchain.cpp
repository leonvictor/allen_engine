#include "swapchain.hpp"

#include "device.hpp"
#include "resources/image.hpp"

#include <common/maths/maths.hpp>

#include <functional>

namespace aln::vkg
{

void Swapchain::Initialize(Device* pDevice, vk::SurfaceKHR* pSurface, uint32_t windowWidth, uint32_t windowHeight)
{
    m_pDevice = pDevice;
    m_width = windowWidth;
    m_height = windowHeight;
    m_pSurface = pSurface;

    CreateInternal();
}

void Swapchain::Shutdown()
{
    m_vkSwapchain.reset();
}

void Swapchain::TargetWindowResizedCallback(uint32_t width, uint32_t height)
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
    for (auto& callback : m_resizeCallbacks)
    {
        callback(m_width, m_height);
    }

    m_resizeRequired = false;
}

void Swapchain::Present(vk::Semaphore& waitSemaphore)
{
    // TODO: Pull out
    vk::PresentInfoKHR presentInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &m_vkSwapchain.get(),
        .pImageIndices = &m_activeImageIndex,
        .pResults = nullptr, // For checking every individual swap chain results. We only have one so we don't need i,
    };

    // TODO: Register swapchain renderers,
    // and add a callback to resize them
    auto result = m_pDevice->GetGraphicsQueue().GetVkQueue().presentKHR(presentInfo);
    // TODO: Shoud this happen in swapchain directly ?
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_resizeRequired)
    {
        m_resizeRequired = true;
    }
    else if (result != vk::Result::eSuccess)
    {
        assert(false); // Failed to present swap chain image.
    }
}

void Swapchain::CreateInternal()
{
    // Create the swapchain; reusing the info if it has already been created
    // TODO: make sure oldSwapchain is destroyed with RAII
    vk::UniqueSwapchainKHR oldSwapchain = std::move(m_vkSwapchain);

    auto createInfo = CreateInfo(&oldSwapchain.get());
    m_vkSwapchain = m_pDevice->GetVkDevice().createSwapchainKHRUnique(createInfo).value;
}

vk::SwapchainCreateInfoKHR Swapchain::CreateInfo(vk::SwapchainKHR* pOldSwapchain)
{
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

    vk::SwapchainCreateInfoKHR createInfo = {
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
        .clipped = vk::True, // We don't care about the color of obscured pixels (ex: if another window is on top)
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
            .width = Maths::Clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            .height = Maths::Clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
        };

        return extent;
    }
}

vk::SurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const Vector<vk::SurfaceFormatKHR>& availableFormats, const vk::Format& desiredFormat)
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

vk::PresentModeKHR Swapchain::ChoosePresentMode(const Vector<vk::PresentModeKHR>& availableModes)
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