#pragma once

#include "../resources/image.hpp"

#include <vulkan/vulkan.hpp>

namespace aln
{

/// @brief Holds render target info and objects.
struct RenderTarget
{
    resources::Image m_multisamplingImage;
    resources::Image m_depthImage;
    resources::Image m_resolveImage;
    vk::UniqueFramebuffer m_framebuffer;

    vk::UniqueSemaphore m_renderFinished; // Signaled when the render is done / waited upon by the present engine before presenting
};
} // namespace aln