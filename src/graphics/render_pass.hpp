#pragma once

#include <vulkan/vulkan.hpp>

#include "resources/image.hpp"
#include "subpass.hpp"

#include <assert.h>

namespace vkg
{
/// @brief Wrapper around the vulkan render pass object. Also acts as a sort of factory.
class RenderPass
{
    enum State
    {
        Uninitialized,
        Initialized,
    };

  public:
    RenderPass() {}

    // Possible feature: Put back a callback logic if and when we make the creation process modular
    // (i.e. renderpass watches the target swapchain and automatically updates when it changes.)
    // This was working ok until we merged the creation process in the constructor, making passing "this" a problem.
    RenderPass(std::shared_ptr<Device> pDevice, int width, int height);

    /// @brief A renderpass is considered initialized as long as it's underlying vulkan renderpass is created.
    bool IsInitialized() const { return m_status == State::Initialized; }

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
    void Begin(vk::CommandBuffer& commandBuffer, vk::Framebuffer& framebuffer);

    void End(vk::CommandBuffer& cb);

    inline vk::RenderPass& GetVkRenderPass() { return m_vkRenderPass.get(); }

    // TODO: This is a lazy solution
    void Resize(int width, int height);

  private:
    std::shared_ptr<Device> m_pDevice;
    std::array<vk::ClearValue, 2> m_clearValues;

    State m_status = State::Uninitialized;

    std::vector<Subpass> m_subpasses;
    std::vector<vk::SubpassDependency> m_subpassDependencies;
    std::vector<vk::AttachmentDescription> m_attachmentDescriptions;

    uint32_t m_width, m_height;

    // Wrapped vulkan renderpass
    vk::UniqueRenderPass m_vkRenderPass;

    /// @brief (Re-)Create the render pass and its inner state objects (i.e.intermediary render targets)
    /// Called whenever the rendering target change sizes.
    void CreateInternal(uint32_t width, uint32_t height);
};
} // namespace vkg