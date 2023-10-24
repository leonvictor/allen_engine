#include "render_pass.hpp"

#include "render_engine.hpp"

namespace aln
{

RenderPass::RenderPass(RenderEngine* pDevice, uint32_t width, uint32_t height)
    : m_pRenderEngine(pDevice),
      m_width(width),
      m_height(height)
{
    assert(pDevice);

    // Default clear values.
    // TODO: Make it possible to customize clear values
    m_clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    m_clearValues[1].depthStencil = {
        .depth = 1.0f,
        .stencil = 0,
    };
}

void RenderPass::Create()
{
    assert(!m_renderPass);
    CreateInternal(m_width, m_height);
}

void RenderPass::Shutdown()
{
    m_pRenderEngine->GetVkDevice().destroyRenderPass(m_renderPass);
}

/// @brief Add a color attachment to this render pass, and return its index.
int RenderPass::AddColorAttachment(vk::Format format)
{
    vk::AttachmentDescription colorAttachment = {
        .format = format,
        .samples = m_pRenderEngine->GetMSAASamples(),
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
    vk::AttachmentDescription colorAttachmentResolve = {
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
    vk::AttachmentDescription depthAttachment = {
        .format = m_pRenderEngine->FindDepthFormat(),
        .samples = m_pRenderEngine->GetMSAASamples(),
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
void RenderPass::Begin(RenderPass::Context& ctx)
{
    assert(m_renderPass);

    m_clearValues[0].color = {
        ctx.backgroundColor.m_red / 255.0f,
        ctx.backgroundColor.m_green / 255.0f,
        ctx.backgroundColor.m_blue / 255.0f,
        1.0f,
    };

    vk::RenderPassBeginInfo renderPassInfo = {
        .renderPass = m_renderPass,
        .framebuffer = ctx.framebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = {m_width, m_height},
        },
        .clearValueCount = static_cast<uint32_t>(m_clearValues.size()),
        .pClearValues = m_clearValues.data(),
    };

    ctx.commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void RenderPass::End(vk::CommandBuffer& cb)
{
    cb.endRenderPass();
}

// TODO: This is a lazy solution
void RenderPass::Resize(uint32_t width, uint32_t height)
{
    CreateInternal(width, height);
}

/// @brief (Re-)Create the render pass and its inner state objects (i.e.intermediary render targets)
/// Called whenever the rendering target change sizes.
void RenderPass::CreateInternal(uint32_t width, uint32_t height)
{
    assert(m_pRenderEngine);

    m_width = width;
    m_height = height;

    // Aggregate subpasses descriptions
    Vector<vk::SubpassDescription> subpassDescriptions;
    for (auto& s : m_subpasses)
    {
        subpassDescriptions.push_back(s.GetDescription());
    }

    vk::RenderPassCreateInfo renderPassInfo = {
        .attachmentCount = static_cast<uint32_t>(m_attachmentDescriptions.size()),
        .pAttachments = m_attachmentDescriptions.data(),
        .subpassCount = static_cast<uint32_t>(subpassDescriptions.size()),
        .pSubpasses = subpassDescriptions.data(),
        .dependencyCount = static_cast<uint32_t>(m_subpassDependencies.size()),
        .pDependencies = m_subpassDependencies.data(),
    };

    m_renderPass = m_pRenderEngine->GetVkDevice().createRenderPass(renderPassInfo).value;
}
} // namespace aln