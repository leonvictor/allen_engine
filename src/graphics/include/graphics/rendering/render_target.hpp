#pragma once

#include "../resources/image.hpp"

#include <vulkan/vulkan.hpp>

namespace aln
{

/// @brief Holds render target info and objects.
struct RenderTarget
{
    GPUImage m_multisamplingImage;
    GPUImage m_depthImage;
    GPUImage m_resolveImage;
    vk::Framebuffer m_framebuffer;

    vk::Semaphore m_renderFinished; // Signaled when the render is done / waited upon by the present engine before presenting
};
} // namespace aln