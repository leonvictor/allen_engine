#pragma once

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{
class Subpass
{
    friend class RenderPass;

  private:
    // The only way to construct a subpass is through a renderpass AddSubpass method.
    Subpass() {}

    Vector<vk::AttachmentReference> m_colorAttachments;
    Vector<vk::AttachmentReference> m_depthAttachments;
    Vector<vk::AttachmentReference> m_resolveAttachments;

    vk::PipelineBindPoint m_bindPoint = vk::PipelineBindPoint::eGraphics;

  public:
    void ReferenceColorAttachment(int attachmentIndex, vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal);
    void ReferenceDepthAttachment(int attachmentIndex, vk::ImageLayout layout = vk::ImageLayout::eDepthStencilAttachmentOptimal);
    void ReferenceResolveAttachment(int attachmentIndex, vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal);

    inline void SetPipelineBIndPoint(vk::PipelineBindPoint bindPoint) { m_bindPoint = bindPoint; }

    inline vk::SubpassDescription GetDescription()
    {
        vk::SubpassDescription desc = {
            .pipelineBindPoint = m_bindPoint,
            .colorAttachmentCount = m_colorAttachments.size(),
            .pColorAttachments = m_colorAttachments.data(),
            .pResolveAttachments = m_resolveAttachments.data(),
            .pDepthStencilAttachment = m_depthAttachments.data(),
        };

        return desc;
    }
};

} // namespace aln