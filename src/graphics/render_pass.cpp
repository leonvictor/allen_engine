#include "render_pass.hpp"

namespace vkg
{

RenderPass::RenderPass(std::shared_ptr<Device> pDevice, int width, int height)
{
    assert(pDevice);

    m_pDevice = pDevice;
    m_width = width;
    m_height = height;

    // Default clear values.
    // TODO: Make it possible to customize clear values
    m_clearValues[0].color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
    m_clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
}

void RenderPass::Create()
{
    assert(!IsInitialized());
    CreateInternal(m_width, m_height);
}

/// @brief Add a color attachment to this render pass, and return its index.
int RenderPass::AddColorAttachment(vk::Format format)
{
    vk::AttachmentDescription colorAttachment{
        .format = format,
        .samples = m_pDevice->GetMSAASamples(),
        // Color and depth data
        .loadOp = vk::AttachmentLoadOp::eClear,   // What to do with the data before ...
        .storeOp = vk::AttachmentStoreOp::eStore, // ... and after rendering
        // Stencil data
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare, // We don't have stencils for now
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        // Images need to be transitioned to specific layout that are suitable for the op that they're going to be involved in next.
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eColorAttachmentOptimal,
    };

    m_attachmentDescriptions.push_back(colorAttachment);
    return m_attachmentDescriptions.size();
}

int RenderPass::AddColorResolveAttachment(vk::Format format, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout)
{
    vk::AttachmentDescription colorAttachmentResolve{
        .format = format,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = initialLayout,
        .finalLayout = finalLayout,
    };

    m_attachmentDescriptions.push_back(colorAttachmentResolve);
    return m_attachmentDescriptions.size();
}

int RenderPass::AddDepthAttachment()
{
    vk::AttachmentDescription depthAttachment{
        .format = m_pDevice->FindDepthFormat(),
        .samples = m_pDevice->GetMSAASamples(),
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare, // Depth data is not used after drawing has finished
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    m_attachmentDescriptions.push_back(depthAttachment);
    return m_attachmentDescriptions.size();
}

/// @brief Register a new subpass and return it for customization.
Subpass& RenderPass::AddSubpass()
{
    auto& subpass = m_subpasses.emplace_back(Subpass());
    return subpass;
}

/// TODO: Choose API: either we pass the full vk struct to this function, or partially build it here
void RenderPass::AddSubpassDependency(vk::SubpassDependency dependency)
{
    m_subpassDependencies.push_back(dependency);
}

/// @brief Begin a render pass.
void RenderPass::Begin(vk::CommandBuffer& commandBuffer, vk::Framebuffer& framebuffer)
{
    assert(IsInitialized());

    vk::RenderPassBeginInfo renderPassInfo{
        .renderPass = m_vkRenderPass.get(),
        .framebuffer = framebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = {m_width, m_height},
        },
        .clearValueCount = static_cast<uint32_t>(m_clearValues.size()),
        .pClearValues = m_clearValues.data(),
    };
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void RenderPass::End(vk::CommandBuffer& cb)
{
    cb.endRenderPass();
}

// TODO: This is a lazy solution
void RenderPass::Resize(int width, int height)
{
    CreateInternal(width, height);
}

/// @brief (Re-)Create the render pass and its inner state objects (i.e.intermediary render targets)
/// Called whenever the rendering target change sizes.
void RenderPass::CreateInternal(uint32_t width, uint32_t height)
{
    assert(m_pDevice);
    m_width = width;
    m_height = height;

    // Aggregate subpasses descriptions
    std::vector<vk::SubpassDescription> vkSubpasses;
    for (auto& s : m_subpasses)
    {
        vkSubpasses.push_back(s.GetDescription());
    }

    vk::RenderPassCreateInfo renderPassInfo{
        .attachmentCount = static_cast<uint32_t>(m_attachmentDescriptions.size()),
        .pAttachments = m_attachmentDescriptions.data(),
        .subpassCount = static_cast<uint32_t>(vkSubpasses.size()),
        .pSubpasses = vkSubpasses.data(),
        .dependencyCount = static_cast<uint32_t>(m_subpassDependencies.size()),
        .pDependencies = m_subpassDependencies.data(),
    };

    m_vkRenderPass = m_pDevice->GetVkDevice().createRenderPassUnique(renderPassInfo);
    m_status = State::Initialized;
}
} // namespace vkg