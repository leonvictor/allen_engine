#include "swapchain.hpp"

#include "render_engine.hpp"
#include "resources/image.hpp"

#include <common/maths/maths.hpp>

#include <functional>

namespace aln
{

void Swapchain::Initialize(RenderEngine* pRenderEngine, vk::SurfaceKHR& surface)
{
    assert(pRenderEngine != nullptr && surface);

    m_pRenderEngine = pRenderEngine;
    m_pSurface = &surface;

    CreateInternal();

    // Retrieve swapchain images
    uint32_t swapchainImageCount;
    m_pRenderEngine->GetVkDevice().getSwapchainImagesKHR(m_swapchain, &swapchainImageCount, nullptr);
    m_images.resize(swapchainImageCount);
    m_pRenderEngine->GetVkDevice().getSwapchainImagesKHR(m_swapchain, &swapchainImageCount, m_images.data());

    m_imageAvailableSemaphores.resize(swapchainImageCount);
    for (auto imageIdx = 0; imageIdx < swapchainImageCount; ++imageIdx)
    {
        m_imageAvailableSemaphores[imageIdx] = m_pRenderEngine->GetVkDevice().createSemaphore({}).value;
        m_pRenderEngine->SetDebugUtilsObjectName(m_imageAvailableSemaphores[imageIdx], "Swapchain Image Available Semaphore (" + std::to_string(imageIdx) + ")");
    }
}

void Swapchain::Shutdown(RenderEngine* pRenderEngine)
{
    for (auto& semaphore : m_imageAvailableSemaphores)
    {
        m_pRenderEngine->GetVkDevice().destroySemaphore(semaphore);
    }
    m_imageAvailableSemaphores.clear();

    m_pRenderEngine->GetVkDevice().destroySwapchainKHR(m_swapchain);
}

void Swapchain::TargetWindowResizedCallback(uint32_t width, uint32_t height)
{
    m_resizeRequired = true;
}

uint32_t Swapchain::AcquireNextImage()
{
    auto& currentFrameImageAvailableSemaphore = m_imageAvailableSemaphores[m_pRenderEngine->GetCurrentFrameIdx()];
    auto result = m_pRenderEngine->GetVkDevice().acquireNextImageKHR(m_swapchain, UINT64_MAX, currentFrameImageAvailableSemaphore, nullptr, &m_activeImageIndex);
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

vk::Semaphore& Swapchain::GetFrameImageAvailableSemaphore()
{
    return m_imageAvailableSemaphores[m_pRenderEngine->GetCurrentFrameIdx()];
}

void Swapchain::Resize(uint32_t width, uint32_t height)
{
    m_pRenderEngine->GetVkDevice().waitIdle();
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
        .pSwapchains = &m_swapchain,
        .pImageIndices = &m_activeImageIndex,
        .pResults = nullptr, // For checking every individual swap chain results. We only have one so we don't need i,
    };

    // TODO: Register swapchain renderers,
    // and add a callback to resize them
    auto result = m_pRenderEngine->GetGraphicsQueue().GetVkQueue().presentKHR(presentInfo);
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
    assert(m_pSurface != nullptr);

    // Create the swapchain; reusing the info if it has already been created
    vk::SwapchainKHR oldSwapchain = std::move(m_swapchain);

    auto swapchainSupport = m_pRenderEngine->GetSwapchainSupport();
    auto presentMode = ChoosePresentMode(swapchainSupport.presentModes);
    m_surfaceFormat = ChooseSurfaceFormat(swapchainSupport.formats); // Defaults to B8G8R8A8Srgb

    // TODO: Reuse current extent if an old swapchain is provided
    // TODO: width, height ? -> maybe grab from a window ?
    m_extent = ChooseExtent(swapchainSupport.capabilities, m_width, m_height);

    uint32_t imageCount = m_pRenderEngine->GetFrameQueueSize();
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
        .oldSwapchain = oldSwapchain,
    };

    uint32_t queueFamilyIndices[] = {
        m_pRenderEngine->GetGraphicsQueue().GetFamilyIndex(),
        m_pRenderEngine->GetPresentQueue().GetFamilyIndex(),
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

    m_swapchain = m_pRenderEngine->GetVkDevice().createSwapchainKHR(createInfo).value;
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
} // namespace aln