#pragma once

#include "resources/image.hpp"
#include "subpass.hpp"

#include <common/colors.hpp>
#include <common/containers/array.hpp>

#include <vulkan/vulkan.hpp>

#include <assert.h>

namespace aln
{
/// @brief Wrapper around the vulkan render pass object. Also acts as a sort of factory.
class RenderPass
{
  public:
    struct Context
    {
        vk::CommandBuffer& commandBuffer;
        vk::Framebuffer& framebuffer;
        aln::RGBAColor backgroundColor = {0, 0, 0, 255};
    };

  private:
    RenderEngine* m_pRenderEngine;
    Array<vk::ClearValue, 2> m_clearValues;

    Vector<Subpass> m_subpasses;
    Vector<vk::SubpassDependency> m_subpassDependencies;
    Vector<vk::AttachmentDescription> m_attachmentDescriptions;

    uint32_t m_width, m_height;

    // Wrapped vulkan renderpass
    vk::UniqueRenderPass m_vkRenderPass;

  private:
    /// @brief (Re-)Create the render pass and its inner state objects (i.e.intermediary render targets)
    /// Called whenever the rendering target change sizes.
    void CreateInternal(uint32_t width, uint32_t height);

  public:
    RenderPass() {}

    // Possible feature: Put back a callback logic if and when we make the creation process modular
    // (i.e. renderpass watches the target swapchain and automatically updates when it changes.)
    // This was working ok until we merged the creation process in the constructor, making passing "this" a problem.
    RenderPass(RenderEngine* pDevice, uint32_t width, uint32_t height);

    void Shutdown()
    {
        m_vkRenderPass.reset();        
    }

    /// @brief Create the renderpass.
    void Create();

    /// @brief Add a color attachment to this render pass, and return its index.
    int AddColorAttachment(vk::Format format);

    int AddColorResolveAttachment(vk::Format format, vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined, vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR);

    int AddDepthAttachment();

    /// @brief Register a new subpass and return it for customization.
    Subpass& AddSubpass();

    /// TODO: Choose API: either we pass the full vk struct to this function, or partially build it here
    void AddSubpassDependency(vk::SubpassDependency dependency);

    /// @brief Begin a render pass.
    void Begin(RenderPass::Context& ctx);

    void End(vk::CommandBuffer& cb);

    inline vk::RenderPass& GetVkRenderPass() { return m_vkRenderPass.get(); }

    // TODO: This is a lazy solution
    void Resize(uint32_t width, uint32_t height);
};
} // namespace aln