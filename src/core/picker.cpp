#include <vulkan/vulkan.hpp>

#include "image.cpp"
#include "pipeline.cpp"
#include "device.hpp"
#include "../scene_object.cpp"
#include "commandpool.cpp"
#include "context.hpp"

class Picker
{
private:
    vk::RenderPass renderPass;
    core::Image image;
    core::Image depthImage;
    vk::Framebuffer framebuffer;
    core::Pipeline pipeline;
    vk::CommandBuffer commandBuffer;
    vk::Fence renderFinished;
    uint32_t height, width;
    vk::Format colorImageFormat;

    std::shared_ptr<core::Device> device;
    std::shared_ptr<core::Context> context;

    void createFence()
    {
        vk::FenceCreateInfo fenceInfo;
        renderFinished = device->logicalDevice.createFence(fenceInfo);
    }

    void createCommandBuffer(const core::CommandPool &commandPool)
    {
        commandBuffer = commandPool.allocateCommandBuffers(1)[0];
    }

    void createFramebuffer()
    {
        std::array<vk::ImageView, 2> attachments = {
            image.view,
            depthImage.view,
        };

        vk::FramebufferCreateInfo fbInfo;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = width;
        fbInfo.height = height;
        fbInfo.layers = 1;

        framebuffer = device->logicalDevice.createFramebuffer(fbInfo);
    }

    void createRessources()
    {
        vk::Format dFormat = device->findDepthFormat();

        depthImage = core::Image(device, width, height, 1, vk::SampleCountFlagBits::e1,
                                 dFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 vk::ImageAspectFlagBits::eDepth);

        image = core::Image(device, width, height, 1, vk::SampleCountFlagBits::e1, colorImageFormat,
                            vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eDeviceLocal,
                            vk::ImageAspectFlagBits::eColor);
#ifndef NDEBUG
        device->setDebugUtilsObjectName(depthImage.image, "Picker depth image");
        device->setDebugUtilsObjectName(image.image, "Picker color image");
#endif
    }

    void createObjectDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding;
        uboLayoutBinding.binding = 0;         // The binding used in the shader
        uboLayoutBinding.descriptorCount = 1; // Number of values in the array
        uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Image sampling related stuff.

        std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};

        vk::DescriptorSetLayoutCreateInfo createInfo{{}, (uint32_t)bindings.size(), bindings.data()};

        descriptorSetLayout = device->logicalDevice.createDescriptorSetLayout(createInfo);
    }

    void createRenderPass()
    {
        vk::AttachmentDescription colorAttachment;
        colorAttachment.format = colorImageFormat;
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference colorAttachmentRef;
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentDescription depthAttachment;
        depthAttachment.format = device->findDepthFormat();
        depthAttachment.samples = vk::SampleCountFlagBits::e1;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference depthAttachmentRef;
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // Add a subpass dependency to ensure the render pass will wait for the right stage
        // We need to wait for the image to be acquired before transitionning to it
        vk::SubpassDependency subpassDependency;
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;                                 // The implicit subpass before or after the render pass
        subpassDependency.dstSubpass = 0;                                                   // Target subpass index (we have only one)
        subpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // Stage to wait on
        subpassDependency.srcAccessMask = vk::AccessFlagBits(0);
        subpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &subpassDependency;

        renderPass = device->logicalDevice.createRenderPass(renderPassInfo);
#ifndef NDEBUG
        device->setDebugUtilsObjectName(renderPass, "Picker renderpass");
#endif
    }

    void createPipeline()
    {
        core::PipelineFactory factory = core::PipelineFactory(device);
        factory.setRenderPass(renderPass);
        factory.setExtent(vk::Extent2D{width, height});
        factory.addDynamicState(vk::DynamicState::eScissor);
        factory.multisample.rasterizationSamples = vk::SampleCountFlagBits::e1; // TODO: set all of those at once.

        factory.registerShader("shaders/picker.vert.spv", vk::ShaderStageFlagBits::eVertex);
        factory.registerShader("shaders/picker.frag.spv", vk::ShaderStageFlagBits::eFragment);
        pipeline = factory.create(std::vector<vk::DescriptorSetLayout>({descriptorSetLayout}));

#ifndef NDEBUG
        device->setDebugUtilsObjectName(pipeline.graphicsPipeline, "Picker graphics Pipeline");
#endif
        // TODO: Create object picking pipeline
        //  * Use a small viewport of around the cursor. We need to be able to specify the viewport dim when drawing
    }

public:
    vk::DescriptorSetLayout descriptorSetLayout;

    void setup(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, const core::CommandPool &commandPool)
    {
        this->device = device;
        this->context = context;

        width = 800;
        height = 600;
        colorImageFormat = vk::Format::eR8G8B8A8Unorm;
        createRessources();
        createObjectDescriptorSetLayout();
        createRenderPass();
        createFramebuffer();
        createPipeline();
        createCommandBuffer(commandPool);
        createFence();
    }

    void destroy()
    {
        image.destroy();
        depthImage.destroy();

        pipeline.destroy();
        device->logicalDevice.destroyRenderPass(renderPass);
        device->logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);
        device->logicalDevice.destroyFence(renderFinished);
        device->logicalDevice.destroyFramebuffer(framebuffer);
    }

    void render(std::vector<SceneObject> models, glm::vec2 target)
    {
        // TODO: Color is mesh-wide and should be passed by uniform
        // TODO: Restrict the info attached to each vertex. We only need the pos.
        // TODO: Handle out of window target
        commandBuffer.begin(vk::CommandBufferBeginInfo());

        // Start a render pass.
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea.extent = vk::Extent2D(width, height); // TODO ?
        // renderPassInfo.renderArea.offset = vk::Offset2D{target.x, target.y}; // TODO -3 ?
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0}; // TODO -3 ?

        std::array<vk::ClearValue, 2> clearValues = {};
        clearValues[0].color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.graphicsPipeline);

        vk::Rect2D scissor = {
            // vk::Offset2D {target.x, target.y},
            vk::Offset2D{0, 0},
            vk::Extent2D{width, height}
            // extent
        };

        commandBuffer.setScissor(0, scissor);

        for (auto model : models)
        {
            commandBuffer.bindVertexBuffers(0, model.mesh.vertexBuffer.buffer, vk::DeviceSize{0});
            commandBuffer.bindIndexBuffer(model.mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, model.colorDescriptorSet, nullptr);
            commandBuffer.drawIndexed(model.mesh.indices.size(), 1, 0, 0, 0);
        }

        commandBuffer.endRenderPass();
        commandBuffer.end();

        // TODO: Pack the submit with the others.
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        device->graphicsQueue.submit(submitInfo, renderFinished);
    }

    glm::vec3 pickColor(std::vector<SceneObject> models, glm::vec2 target)
    {
        // TODO: Call that elsewhere
        render(models, target);

        device->logicalDevice.waitForFences(renderFinished, VK_TRUE, UINT64_MAX);
        device->logicalDevice.resetFences(renderFinished);

        core::Image stagingImage = core::Image(device, width, height, 1, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eTransferDst,
                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent); // TODO: OPTIMIZE We don't need a view

        // TODO: OPTIMIZE Use a single command buffer to do all necessary operations
        image.transitionLayout(context, vk::ImageLayout::eTransferSrcOptimal);
        stagingImage.transitionLayout(context, vk::ImageLayout::eTransferDstOptimal);

        bool colorSwizzle = false;

        if (device->supportsBlittingToLinearImages())
        {
            image.blit(context, stagingImage, width, height);
        }
        else
        {
            image.copyTo(context, stagingImage, width, height);

            // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
            // Check if source is BGR
            // Note: Not complete, only contains most common and basic BGR surface formats for demonstation purposes
            std::vector<vk::Format> formatsBGR = {vk::Format::eB8G8R8A8Srgb, vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Snorm};
            colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), image.format) != formatsBGR.end());
        }

        stagingImage.transitionLayout(context, vk::ImageLayout::eGeneral);
        image.transitionLayout(context, vk::ImageLayout::eColorAttachmentOptimal);

        // TODO: Grab only the value in the middle pixel
        stagingImage.save("debug.ppm", colorSwizzle);
        glm::vec3 pixelValue = stagingImage.pixelAt(target.x, target.y);

        stagingImage.destroy();
        return pixelValue;
    }
};