#include "subpass.hpp"

namespace aln
{
void Subpass::ReferenceColorAttachment(int attachmentIndex, vk::ImageLayout layout)
{
    vk::AttachmentReference ref;
    ref.attachment = attachmentIndex;
    ref.layout = layout;

    m_colorAttachments.push_back(ref);
}

void Subpass::ReferenceDepthAttachment(int attachmentIndex, vk::ImageLayout layout)
{
    vk::AttachmentReference ref;
    ref.attachment = attachmentIndex;
    ref.layout = layout;

    m_depthAttachments.push_back(ref);
}

void Subpass::ReferenceResolveAttachment(int attachmentIndex, vk::ImageLayout layout)
{
    vk::AttachmentReference ref;
    ref.attachment = attachmentIndex;
    ref.layout = layout;

    m_resolveAttachments.push_back(ref);
}
} // namespace aln