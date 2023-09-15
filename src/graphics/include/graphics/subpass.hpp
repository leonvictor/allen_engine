#pragma once

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

namespace aln::vkg
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
        vk::SubpassDescription desc;
        desc.pipelineBindPoint = m_bindPoint;
        desc.colorAttachmentCount = m_colorAttachments.size();
        desc.pColorAttachments = m_colorAttachments.data(); // The index of the attachment is directly referenced in the fragment shader ( layout(location = 0) )...
        desc.pResolveAttachments = m_resolveAttachments.data();
        desc.pDepthStencilAttachment = m_depthAttachments.data();
        return desc;
    }
};

} // namespace aln::vkg