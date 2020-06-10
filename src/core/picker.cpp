#include <vulkan/vulkan.hpp>
#include "image.cpp"
#include "pipeline.cpp"

class ObjectPicker
{
public:
    void init()
    {
        createPickerRessources();
        createPickerObjectDescriptorSetLayout();
        createPickerRenderPass();
        createFramebuffer();
        createPickerPipeline();
        createCommandBuffer();
    }

private:
    vk::RenderPass renderPass;
    core::Image image;
    core::Image depthImage;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::Framebuffer framebuffer;
    core::Pipeline pipeline;
    vk::CommandBuffer commandBuffer;

    void createCommandBuffer()
    {
        commandBuffer = commandPool.allocateCommandBuffers(1)[0];
    }

    void createFramebuffer()
    {
        std::array<vk::ImageView, 2> attachments = {
            picker.image.view,
            picker.depthImage.view,
        };

        vk::FramebufferCreateInfo fbInfo;
        fbInfo.renderPass = picker.renderPass; // TODO: Different renderpass
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = 3;
        fbInfo.height = 3;
        fbInfo.layers = 1;

        picker.framebuffer = device->logicalDevice.createFramebuffer(fbInfo);
    }

    void createPickerRessources()
    {
        vk::Format format = device->findDepthFormat();

        picker.depthImage = core::Image(device, 3, 3, 1, vk::SampleCountFlagBits::e1,
                                        format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                        vk::ImageAspectFlagBits::eDepth);

        picker.image = core::Image(device, 3, 3, 1, vk::SampleCountFlagBits::e1, this->imageFormat,
                                   vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                   vk::ImageAspectFlagBits::eColor);
        // TODO: Check if we need transient usage flag
    }

    void createPickerObjectDescriptorSetLayout()
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding;
        uboLayoutBinding.binding = 0;         // The binding used in the shader
        uboLayoutBinding.descriptorCount = 1; // Number of values in the array
        uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        // We need to access the ubo in the fragment shader aswell now (because it contains light direction)
        // TODO: There is probably a cleaner way (a descriptor for all light sources for example ?)
        uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Image sampling related stuff.

        std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};

        vk::DescriptorSetLayoutCreateInfo createInfo{{}, (uint32_t)bindings.size(), bindings.data()};

        picker.descriptorSetLayout = device->logicalDevice.createDescriptorSetLayout(createInfo);
    }

    void createPickerRenderPass()
    {
        vk::AttachmentDescription colorAttachment;
        colorAttachment.format = imageFormat;
        colorAttachment.samples = vk::SampleCountFlagBits::e1; // TODO: ?
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentDescription depthAttachment;
        depthAttachment.format = device->findDepthFormat();
        depthAttachment.samples = vk::SampleCountFlagBits::e1;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare; // Depth data is not used after drawing has finished
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

        picker.renderPass = device->logicalDevice.createRenderPass(renderPassInfo);
    }

    void createPickerPipeline()
    {
        core::PipelineFactory factory = core::PipelineFactory(device);
        factory.setRenderPass(picker.renderPass);
        factory.setExtent(extent);
        factory.addDynamicState(vk::DynamicState::eViewport);
        factory.multisample.rasterizationSamples = vk::SampleCountFlagBits::e1; // TODO: set all of those at once.

        // TODO: Specific shaders
        factory.registerShader("shaders/picker.vert.spv", vk::ShaderStageFlagBits::eVertex);
        factory.registerShader("shaders/picker.frag.spv", vk::ShaderStageFlagBits::eFragment);
        picker.pipeline = factory.create(std::vector<vk::DescriptorSetLayout>({picker.descriptorSetLayout}));

        // TODO: Create object picking pipeline
        //  * No actual rendering
        //  * Write vertex color to a specific buffer
        //  * Use a small viewport of around the cursor. We need to be able to specify the viewport dim when drawing
        //  *

        // 1) register simple color shaders
        // 2)
    }
};