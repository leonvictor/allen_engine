#pragma once
#include <vulkan/vulkan.hpp>

namespace vkg
{
class Subpass
{
    friend class RenderPass;

  private:
    // The only way to construct a subpass is through a renderpass AddSubpass method.
    Subpass() {}

    std::vector<vk::AttachmentReference> m_colorAttachments;
    std::vector<vk::AttachmentReference> m_depthAttachments;
    std::vector<vk::AttachmentReference> m_resolveAttachments;

    vk::PipelineBindPoint m_bindPoint = vk::PipelineBindPoint::eGraphics;

  public:
    void ReferenceColorAttachment(int attachmentIndex, vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal)
    {
        vk::AttachmentReference ref;
        ref.attachment = attachmentIndex;
        ref.layout = layout;

        m_colorAttachments.push_back(ref);
    }

    void ReferenceDepthAttachment(int attachmentIndex, vk::ImageLayout layout = vk::ImageLayout::eDepthAttachmentOptimal)
    {
        vk::AttachmentReference ref;
        ref.attachment = attachmentIndex;
        ref.layout = layout;

        m_depthAttachments.push_back(ref);
    }

    void ReferenceResolveAttachment(int attachmentIndex, vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal)
    {
        vk::AttachmentReference ref;
        ref.attachment = attachmentIndex;
        ref.layout = layout;

        m_resolveAttachments.push_back(ref);
    }

    void SetPipelineBIndPoint(vk::PipelineBindPoint bindPoint)
    {
        m_bindPoint = bindPoint;
    }

    vk::SubpassDescription GetDescription()
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

} // namespace vkg