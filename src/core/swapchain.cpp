#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"
#include "context.hpp"
#include "image.cpp"
#include "commandpool.cpp"
#include "pipeline.cpp"
#include "buffer.cpp"
#include <memory>
#include "../skybox.cpp"

#include <array>
#include <GLFW/glfw3.h>
#include "../scene_object.cpp"
#include "picker.cpp"

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace core
{
    // TODO: Rework to use core::Image
    //   * more info: this is problematic when it comes to destruction because
    //     swapchain images shouldn't be deleted manually
    struct SwapchainImage
    {
        vk::Framebuffer framebuffer;
        vk::CommandBuffer commandbuffer;
        vk::Image image;
        vk::ImageView imageView;
        vk::Fence fence;

        void cleanup(vk::Device &device, const vk::CommandPool &commandPool)
        {
            device.destroyFramebuffer(framebuffer);
            device.freeCommandBuffers(commandPool, commandbuffer);
            device.destroyImageView(imageView);
        }
    };

    class Swapchain
    {
    public:
        core::Image colorImage;
        core::Image depthImage;

        struct
        {
            core::Pipeline objects;
            core::Pipeline skybox;
        } pipelines;

        // Object picking
        Picker picker;

        // vk::SurfaceKHR surface;
        vk::SwapchainKHR swapchain;
        vk::PresentInfoKHR presentInfo;

        std::vector<SwapchainImage> images;

        vk::Format imageFormat;
        vk::Extent2D extent;
        vk::RenderPass renderPass;

        std::shared_ptr<core::Context> context; // TODO: Fuse device, context and swapchain somehow
        core::CommandPool commandPool;

        std::vector<vk::Semaphore> imageAvailableSemaphores;
        std::vector<vk::Semaphore> renderFinishedSemaphores;
        std::vector<vk::Fence> inFlightFences;

        int currentFrame = 0;
        bool frame_active = false;
        uint32_t activeFrameIndex;

        // There is probably a better place for those
        vk::DescriptorSetLayout objectsDescriptorSetLayout;
        vk::DescriptorSetLayout lightsDescriptorSetLayout;
        vk::DescriptorSetLayout skyboxDescriptorSetLayout;
        vk::DescriptorPool descriptorPool;

        Swapchain() {}


        void recreate(GLFWwindow *window, core::CommandPool &commandPool, int maxObjects)
        {
            createSwapchain(window);
            createImages();
            createRenderPass();
            createPipelines();
            createDepthResources();
            createColorResources();
            createFramebuffers();
            createDescriptorPool(maxObjects);
            createCommandBuffers(commandPool);

            // TODO : Add descriptor sets and command buffers here
        }

        // TODO: Normalize object construction
        void init(std::shared_ptr<core::Context> context, core::CommandPool &commandPool, GLFWwindow *window, int maxObjects)
        {
            this->context = context;

            createSwapchain(window);
            createImages();
            createDepthResources();
            createColorResources();
            createRenderPass();
            createDescriptorSetLayout();
            createSkyboxDescriptorSetLayout();

            createPipelines();
            createFramebuffers();
            createSyncObjects();
            createDescriptorPool(maxObjects);
            createCommandBuffers(commandPool);
            picker.setup(context, context->device, commandPool);
        }

        void createPipelines()
        {
            core::PipelineFactory factory = core::PipelineFactory(context->device);
            factory.setRenderPass(renderPass);
            factory.setExtent(extent);

            factory.registerShader("shaders/vert.spv", vk::ShaderStageFlagBits::eVertex);
            factory.registerShader("shaders/frag.spv", vk::ShaderStageFlagBits::eFragment);
            pipelines.objects = factory.create(std::vector<vk::DescriptorSetLayout>({lightsDescriptorSetLayout, objectsDescriptorSetLayout}));

            // TODO: Create object picking pipeline
            //  * No actual rendering
            //  * Write vertex color to a specific buffer
            //  * Use a small viewport of around the cursor. We need to be able to specify the viewport dim when drawing
            //  *

            // 1) register simple color shaders
            // 2)

            factory.registerShader("shaders/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
            factory.registerShader("shaders/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);
            factory.depthStencil.depthWriteEnable = VK_FALSE;
            factory.rasterizer.cullMode = vk::CullModeFlagBits::eNone;
            pipelines.skybox = factory.create(std::vector<vk::DescriptorSetLayout>({skyboxDescriptorSetLayout}));

// TODO: Get rid of the double negative
#ifndef NDEBUG
            context->device->setDebugUtilsObjectName(pipelines.skybox.graphicsPipeline, "Skybox Pipeline");
            context->device->setDebugUtilsObjectName(pipelines.objects.graphicsPipeline, "Objects Pipeline");
#endif
        }

        void createSkyboxDescriptorSetLayout()
        {
            std::vector<vk::DescriptorSetLayoutBinding> setsLayoutBindings{
                {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}};

            skyboxDescriptorSetLayout = context->device->logical.get().createDescriptorSetLayout({{}, (uint32_t)setsLayoutBindings.size(), setsLayoutBindings.data()});

#ifndef NDEBUG
            context->device->setDebugUtilsObjectName(skyboxDescriptorSetLayout, "Skybox Descriptor Set Layout");
#endif
        }

        void createFramebuffers()
        {
            for (int i = 0; i < images.size(); i++)
            {
                images[i].framebuffer = createFramebuffer(images[i].imageView, renderPass);
            }
        }

        void createRenderPass()
        {
            assert(context->device);

            vk::AttachmentDescription colorAttachment;
            colorAttachment.format = imageFormat;
            colorAttachment.samples = context->device->msaaSamples;
            // Color and depth data
            colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;   // What to do with the data before ...
            colorAttachment.storeOp = vk::AttachmentStoreOp::eStore; // ... and after rendering
            // Stencil data
            colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare; // We don't have stencils for now
            colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

            // Images need to be transitioned to specific layout that are suitable for the op that they're going to be involved in next.
            colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
            colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

            vk::AttachmentReference colorAttachmentRef;
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            vk::AttachmentDescription depthAttachment;
            depthAttachment.format = context->device->findDepthFormat();
            depthAttachment.samples = context->device->msaaSamples;
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
            colorAttachmentResolve.format = imageFormat;
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
            subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;                                 // The implicit subpass before or after the render pass
            subpassDependency.dstSubpass = 0;                                                   // Target subpass index (we have only one)
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

            renderPass = context->device->logical.get().createRenderPass(renderPassInfo);
        }

        void createCommandBuffers(const core::CommandPool &commandPool)
        {
            // TODO: This has to happen somewhere else
            this->commandPool = commandPool;
            auto commandBuffers = commandPool.allocateCommandBuffers(images.size());

            for (size_t i = 0; i < images.size(); i++)
            {
                images[i].commandbuffer = commandBuffers[i];
            }
        }

        void beginDrawFrame(uint32_t index)
        {
            images[index].commandbuffer.begin(vk::CommandBufferBeginInfo{});

            // Start a render pass.
            vk::RenderPassBeginInfo renderPassInfo;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = images[index].framebuffer;
            renderPassInfo.renderArea.extent = extent;
            renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};

            std::array<vk::ClearValue, 2> clearValues = {};
            clearValues[0].color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            images[index].commandbuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        }

        void endDrawFrame(uint32_t index)
        {
            images[index].commandbuffer.endRenderPass();
            images[index].commandbuffer.end();
        }

        void recordCommandBuffer(uint32_t index, std::vector<SceneObject> models, vk::DescriptorSet lightsDescriptorSet, Skybox &skybox)
        {
            // Skybox
            images[index].commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelines.skybox.layout, 0, skybox.descriptorSet, nullptr);
            images[index].commandbuffer.bindVertexBuffers(0, skybox.mesh.vertexBuffer.buffer, vk::DeviceSize{0});
            images[index].commandbuffer.bindIndexBuffer(skybox.mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
            images[index].commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skybox.graphicsPipeline);
            images[index].commandbuffer.drawIndexed(skybox.mesh.indices.size(), 1, 0, 0, 0);

            // Objects
            images[index].commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.objects.graphicsPipeline);
            images[index].commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelines.objects.layout, 0, lightsDescriptorSet, nullptr);

            for (auto model : models)
            {
                images[index].commandbuffer.bindVertexBuffers(0, model.mesh.vertexBuffer.buffer, vk::DeviceSize{0});
                images[index].commandbuffer.bindIndexBuffer(model.mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
                images[index].commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelines.objects.layout, 1, model.descriptorSet, nullptr);
                images[index].commandbuffer.drawIndexed(model.mesh.indices.size(), 1, 0, 0, 0);
            }
        }

        void recordCommandBuffers(std::vector<SceneObject> models, vk::DescriptorSet lightsDescriptorSet, Skybox &skybox)
        {
            for (size_t i = 0; i < images.size(); i++)
            {
                recordCommandBuffer(i, models, lightsDescriptorSet, skybox);
            }
        }
        
        void createSyncObjects()
        {
            vk::SemaphoreCreateInfo semaphoreInfo;
            vk::FenceCreateInfo fenceInfo;
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled; // Create fences in the signaled state

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                imageAvailableSemaphores.push_back(context->device->logical.get().createSemaphore(semaphoreInfo, nullptr));
                renderFinishedSemaphores.push_back(context->device->logical.get().createSemaphore(semaphoreInfo, nullptr));
                inFlightFences.push_back(context->device->logical.get().createFence(fenceInfo, nullptr));
            }
        }

        // Destroy the parts we need to recreate
        void cleanup()
        {
            colorImage.destroy();
            depthImage.destroy();

            for (auto img : images)
            {
                img.cleanup(context->device->logical.get(), commandPool.pool);
            }

            pipelines.objects.destroy();
            pipelines.skybox.destroy();

            context->device->logical.get().destroyRenderPass(renderPass);

            context->device->logical.get().destroySwapchainKHR(swapchain);

            context->device->logical.get().destroyDescriptorPool(descriptorPool);
        }

        void destroy()
        {
            cleanup();
            picker.destroy();
            
            context->device->logical.get().destroyDescriptorSetLayout(objectsDescriptorSetLayout);
            context->device->logical.get().destroyDescriptorSetLayout(lightsDescriptorSetLayout);
            context->device->logical.get().destroyDescriptorSetLayout(skyboxDescriptorSetLayout);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                context->device->logical.get().destroySemaphore(imageAvailableSemaphores[i]);
                context->device->logical.get().destroySemaphore(renderFinishedSemaphores[i]);
                context->device->logical.get().destroyFence(inFlightFences[i]);
            }
        }

    private:
        void createSwapchain(GLFWwindow *window)
        {
            core::SwapchainSupportDetails swapchainSupport = context->device->getSwapchainSupport(context->surface);
            vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats); // Defaults to B8G8R8A8Srgb
            vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);

            vk::Extent2D extent = chooseSwapExtent(swapchainSupport.capabilities, window);

            uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
            if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
            {
                imageCount = swapchainSupport.capabilities.maxImageCount;
            }

            vk::SwapchainCreateInfoKHR sCreateInfo;
            sCreateInfo.surface = context->surface.get();
            sCreateInfo.minImageCount = imageCount;
            sCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
            sCreateInfo.imageFormat = surfaceFormat.format;
            sCreateInfo.imageExtent = extent;
            sCreateInfo.imageArrayLayers = 1;
            sCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            sCreateInfo.presentMode = presentMode;
            sCreateInfo.clipped = VK_TRUE; // We don't care about the color of obscured pixels (ex: if another window is on top)

            core::QueueFamilyIndices indices = context->device->getQueueFamilyIndices();
            uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            if (indices.graphicsFamily != indices.presentFamily)
            {
                sCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
                sCreateInfo.queueFamilyIndexCount = 2;
                sCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else
            {
                sCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
                sCreateInfo.queueFamilyIndexCount = 0;     // Optionnal
                sCreateInfo.pQueueFamilyIndices = nullptr; // Optionnal
            }

            sCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;
            sCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            sCreateInfo.oldSwapchain = {(VkSwapchainKHR_T *)0}; // TODO : Pass the old swapchain to reuse most of the info.

            swapchain = context->device->logical.get().createSwapchainKHR(sCreateInfo);

            this->extent = extent;
            imageFormat = surfaceFormat.format;
        }

        vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats, const vk::Format& desiredFormat = vk::Format::eB8G8R8A8Srgb)
        {
            // TODO: Refine selection (ex: support bliting to linear tiling format)
            for (const auto &format : availableFormats)
            {
                // Return the first format that supports SRGB color space in 8bit by channels
                if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && format.format == desiredFormat) // TODO: We might want to switch == to & if we have multiple acceptable formats
                {
                    return format;
                }
            }
            return availableFormats[0];
        }

        vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availableModes)
        {
            for (const auto &mode : availableModes)
            {
                if (mode == vk::PresentModeKHR::eMailbox)
                {
                    return mode;
                }
            }
            return vk::PresentModeKHR::eFifo;
        }

        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window)
        {
            if (capabilities.currentExtent.width != UINT32_MAX)
            {
                return capabilities.currentExtent;
            }
            else
            {
                // Some window managers do not specify the resolution (indicated by special max value)
                // In this case, use the resolution that best matches the window within the ImageExtent bounds
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D extent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)};

                extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                return extent;
            }
        }

        void createImages()
        {
            std::vector<vk::Image> imgs = context->device->logical.get().getSwapchainImagesKHR(swapchain);

            for (vk::Image swapImage : imgs)
            {
                vk::ImageView view = core::Image::createImageView(context->device, swapImage, imageFormat, vk::ImageAspectFlagBits::eColor, 1);
                SwapchainImage image = SwapchainImage {
                    nullptr,
                    nullptr,
                    swapImage,
                    view,
                    vk::Fence()
                };
                this->images.push_back(image);
            }
        }

        vk::Framebuffer createFramebuffer(vk::ImageView view, vk::RenderPass renderPass)
        {
            std::array<vk::ImageView, 3> attachments = {
                colorImage.view,
                depthImage.view,
                view};

            vk::FramebufferCreateInfo framebufferInfo;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1; // Nb of layers in image array.
            return context->device->logical.get().createFramebuffer(framebufferInfo);
        }

        void createDepthResources()
        {
            vk::Format format = context->device->findDepthFormat();

            depthImage = core::Image(context->device, extent.width, extent.height, 1, context->device->msaaSamples,
                                     format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                     vk::ImageAspectFlagBits::eDepth);
        }

        void createColorResources()
        {
            colorImage = core::Image(context->device, extent.width, extent.height, 1, context->device->msaaSamples, this->imageFormat,
                                     vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                     vk::ImageAspectFlagBits::eColor);

#ifndef NDEBUG
            context->device->setDebugUtilsObjectName(colorImage.view, "Color Image View");
#endif
        }

        void createDescriptorSetLayout()
        {
            vk::DescriptorSetLayoutBinding uboLayoutBinding;
            uboLayoutBinding.binding = 0;         // The binding used in the shader
            uboLayoutBinding.descriptorCount = 1; // Number of values in the array
            uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            // We need to access the ubo in the fragment shader aswell now (because it contains light direction)
            // TODO: There is probably a cleaner way (a descriptor for all light sources for example ?)
            uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
            uboLayoutBinding.pImmutableSamplers = nullptr; // Image sampling related stuff.

            vk::DescriptorSetLayoutBinding samplerLayoutBinding;
            samplerLayoutBinding.binding = 1;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            samplerLayoutBinding.pImmutableSamplers = nullptr;
            samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment; //It's possible to use texture sampling in the vertex shader as well, for example to dynamically deform a grid of vertices by a heightmap

            vk::DescriptorSetLayoutBinding materialLayoutBinding;
            materialLayoutBinding.binding = 2;
            materialLayoutBinding.descriptorCount = 1;
            materialLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            materialLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {uboLayoutBinding, samplerLayoutBinding, materialLayoutBinding};

            vk::DescriptorSetLayoutCreateInfo createInfo{{}, (uint32_t)bindings.size(), bindings.data()};

            objectsDescriptorSetLayout = context->device->logical.get().createDescriptorSetLayout(createInfo);

            vk::DescriptorSetLayoutBinding lightsLayoutBinding;
            lightsLayoutBinding.binding = 0;
            lightsLayoutBinding.descriptorCount = 1;
            lightsLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            lightsLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            vk::DescriptorSetLayoutCreateInfo lightsCreateInfo;
            lightsCreateInfo.bindingCount = 1;
            lightsCreateInfo.pBindings = &lightsLayoutBinding;

            lightsDescriptorSetLayout = context->device->logical.get().createDescriptorSetLayout(lightsCreateInfo);

#ifndef NDEBUG
            context->device->setDebugUtilsObjectName(objectsDescriptorSetLayout, "Object Descriptor Layout");
            context->device->setDebugUtilsObjectName(lightsDescriptorSetLayout, "Lights Descriptor Set Layout");
#endif
        }

        void createDescriptorPool(int nObjects)
        {
            // TODO: Refactor
            std::array<vk::DescriptorPoolSize, 3> poolSizes;
            
            poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
            poolSizes[0].descriptorCount = (nObjects * 3) + 1; // +1 for skybox. TODO: This should be dynamic
            poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
            poolSizes[1].descriptorCount = nObjects + 1; // +1 TODO Same here
            // poolSizes[2].type = vk::DescriptorType::eUniformBuffer;
            // poolSizes[2].descriptorCount = nObjects; // TODO: Can we fuse the two uniformBuffer pools ? This one is for materials.
            poolSizes[2].type = vk::DescriptorType::eStorageBuffer;
            poolSizes[2].descriptorCount = 1; // For now, only used by lights

            vk::DescriptorPoolCreateInfo createInfo;
            createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            createInfo.pPoolSizes = poolSizes.data();
            createInfo.maxSets = (nObjects * 2) + 2; // TODO: +2 is for lights / skybox. Make it less hardcoded.  * 2 for color picker 

            descriptorPool = context->device->logical.get().createDescriptorPool(createInfo);
        }
    };
}; // namespace core