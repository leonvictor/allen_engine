#pragma once

#include "../imgui_service.hpp"
#include "../pipeline.hpp"
#include "../render_engine.hpp"
#include "../render_pass.hpp"
#include "../resources/buffer.hpp"
#include "../resources/image.hpp"
#include "../swapchain.hpp"
#include "render_target.hpp"

#include <common/colors.hpp>
#include <common/containers/vector.hpp>
#include <common/hash_vector.hpp>

#include <config/path.h>
#include <vulkan/vulkan.hpp>

#include <cstring>

namespace aln
{

// fwd
class StaticMeshComponent;
class SkeletalMeshComponent;
class LightComponent;
class Mesh;

struct RenderContext
{
    aln::RGBAColor backgroundColor = {0, 0, 0, 255};
};

/// @brief Renderer instance used to draw the scenes and the UI.
class IRenderer
{
  protected:
    static constexpr uint32_t MAX_MODELS = 1000;
    static constexpr uint32_t MAX_SKINNING_TRANSFORMS = 255 * 50;

    RenderEngine* m_pRenderEngine;
    RenderPass m_renderpass;

    // TODO: A renderer can be reused for multiples worlds (i.e. game scene + editor previews)
    // But the framebuffers must be different. This means render targets should be held elsewhere
    Vector<RenderTarget> m_renderTargets;

    virtual void CreateInternal(RenderEngine* pRenderEngine, uint32_t width, uint32_t height, vk::Format colorImageFormat) {}

    virtual void Initialize(RenderEngine* pRenderEngine) = 0;
    virtual void Shutdown() = 0;

  public:
    RenderPass& GetRenderPass() { return m_renderpass; }
};
} // namespace aln