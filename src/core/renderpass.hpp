#include <vulkan/vulkan.hpp>
#include "swapchain.cpp"
#include "device.hpp"

namespace core {
class RenderPass {
public:

    vk::RenderPass renderPass;

    void init(core::Device device, core::Swapchain swapchain) {
        this->device = &device;
        this->swapchain = &swapchain;
        
        createRenderPass();
    }

    void destroy() {
        //TODO
    }

    operator vk::RenderPass() {
        return renderPass;
    }

    operator VkRenderPass() { return VkRenderPass(renderPass); }

private:
    core::Device *device;
    core::Swapchain *swapchain;

    void createRenderPass() {
        assert(device);
        assert(swapchain->isInitialized());

        vk::AttachmentDescription colorAttachment;
        colorAttachment.format = swapchain->imageFormat;
        colorAttachment.samples = device->msaaSamples;
        // Color and depth data
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear; // What to do with the data before ...
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore; // ... and after rendering
        // Stencil data
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare; // We don't have stencils for now
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

        // Images need to be transitioned to specific layout that are suitable for the op that they're going to be involved in next.
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentDescription depthAttachment;
        depthAttachment.format = device->findDepthFormat();
        depthAttachment.samples = device->msaaSamples;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare; // Depth data is not used after drawing has finished
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference depthAttachmentRef;
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentDescription colorAttachmentResolve;
        colorAttachmentResolve.format = swapchain->imageFormat;
        colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
        colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;
        colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

        vk::AttachmentReference colorAttachmentResolveRef;
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef; // The index of the attachment is directly referenced in the fragment shader ( layout(location = 0) )...
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;
        
        // Add a subpass dependency to ensure the render pass will wait for the right stage
        // We need to wait for the image to be acquired before transitionning to it
        vk::SubpassDependency subpassDependency;
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // The implicit subpass before or after the render pass
        subpassDependency.dstSubpass = 0; // Target subpass index (we have only one)
        subpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // Stage to wait on
        subpassDependency.srcAccessMask = vk::AccessFlagBits(0);
        subpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        std::array<vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &subpassDependency;

        renderPass = device->logicalDevice.createRenderPass(renderPassInfo);
    }
};
};