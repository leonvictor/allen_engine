#pragma once

#include <vulkan/vulkan.hpp>

namespace aln::vkg::render
{

/// @brief Holds render target info and objects.
struct RenderTarget
{
    uint32_t index;
    vk::UniqueFramebuffer framebuffer;
    vk::UniqueCommandBuffer commandBuffer;
    vk::Fence fence;
};
} // namespace vkg