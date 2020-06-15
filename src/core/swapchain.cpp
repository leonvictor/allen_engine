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

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace core
{
    // TODO: Rework to use core::Image
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
        // TODO: Refactor out of swapchain.
        struct Picker
        {
            vk::RenderPass renderPass;
            core::Image image;
            core::Image depthImage;
            vk::DescriptorSetLayout descriptorSetLayout;
            vk::Framebuffer framebuffer;
            core::Pipeline pipeline;
            vk::CommandBuffer commandBuffer;
            vk::Fence renderFinished;
            int height, width;
            vk::Format colorImageFormat;
        } picker;

        vk::SurfaceKHR surface;
        vk::SwapchainKHR swapchain;
        vk::PresentInfoKHR presentInfo;

        std::vector<SwapchainImage> images;

        vk::Format imageFormat;
        vk::Extent2D extent;
        vk::RenderPass renderPass;

        std::shared_ptr<core::Device> device;
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

        void createSurface(std::shared_ptr<core::Context> context, GLFWwindow *window)
        {
            this->context = context; // TODO: berk!
            VkSurfaceKHR pSurface = VkSurfaceKHR(surface);
            if (glfwCreateWindowSurface(context->instance.get(), window, nullptr, &pSurface) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create window surface.");
            }
            surface = vk::SurfaceKHR(pSurface);
        }

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
        void init(std::shared_ptr<core::Device> device, core::CommandPool &commandPool, GLFWwindow *window, int maxObjects)
        {
            this->device = device;

            assert(surface);
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
            setupPicker();
        }

        void createPipelines()
        {
            core::PipelineFactory factory = core::PipelineFactory(device);
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
            device->setDebugUtilsObjectName(pipelines.skybox.graphicsPipeline, "Skybox Pipeline");
            device->setDebugUtilsObjectName(pipelines.objects.graphicsPipeline, "Objects Pipeline");
#endif
        }

        void createSkyboxDescriptorSetLayout()
        {
            std::vector<vk::DescriptorSetLayoutBinding> setsLayoutBindings{
                {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}};

            skyboxDescriptorSetLayout = device->logicalDevice.createDescriptorSetLayout({{}, (uint32_t)setsLayoutBindings.size(), setsLayoutBindings.data()});

#ifndef NDEBUG
            device->setDebugUtilsObjectName(skyboxDescriptorSetLayout, "Skybox Descriptor Set Layout");
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
            assert(device);

            vk::AttachmentDescription colorAttachment;
            colorAttachment.format = imageFormat;
            colorAttachment.samples = device->msaaSamples;
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

            renderPass = device->logicalDevice.createRenderPass(renderPassInfo);
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
            // TODO: Refactor in modern c++
            imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            vk::SemaphoreCreateInfo semaphoreInfo;
            vk::FenceCreateInfo fenceInfo;
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled; // Create fences in the signaled state

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                imageAvailableSemaphores[i] = device->logicalDevice.createSemaphore(semaphoreInfo, nullptr);
                renderFinishedSemaphores[i] = device->logicalDevice.createSemaphore(semaphoreInfo, nullptr);
                inFlightFences[i] = device->logicalDevice.createFence(fenceInfo, nullptr);
            }
        }

        // Destroy the parts we need to recreate
        void cleanup()
        {
            colorImage.destroy();
            depthImage.destroy();

            for (auto img : images)
            {
                img.cleanup(device->logicalDevice, commandPool.pool);
            }

            pipelines.objects.destroy();
            pipelines.skybox.destroy();

            device->logicalDevice.destroyRenderPass(renderPass);

            device->logicalDevice.destroySwapchainKHR(swapchain);

            device->logicalDevice.destroyDescriptorPool(descriptorPool);
        }

        void destroy()
        {
            cleanup();

            device->logicalDevice.destroyDescriptorSetLayout(objectsDescriptorSetLayout);
            device->logicalDevice.destroyDescriptorSetLayout(lightsDescriptorSetLayout);
            device->logicalDevice.destroyDescriptorSetLayout(skyboxDescriptorSetLayout);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                device->logicalDevice.destroySemaphore(imageAvailableSemaphores[i]);
                device->logicalDevice.destroySemaphore(renderFinishedSemaphores[i]);
                device->logicalDevice.destroyFence(inFlightFences[i]);
            }
        }

        void renderObjectPickerImage(std::vector<SceneObject> models, glm::vec2 target) {
            // TODO: Color is mesh-wide and should be passed by uniform
            // TODO: Restrict the info attached to each vertex. We only need the pos.
            // TODO: Handle out of window target
            picker.commandBuffer.begin(vk::CommandBufferBeginInfo());

            // Start a render pass.
            vk::RenderPassBeginInfo renderPassInfo;
            renderPassInfo.renderPass = picker.renderPass;
            renderPassInfo.framebuffer = picker.framebuffer;
            renderPassInfo.renderArea.extent = vk::Extent2D(picker.width, picker.height); // TODO ?
            // renderPassInfo.renderArea.offset = vk::Offset2D{target.x, target.y}; // TODO -3 ?
            renderPassInfo.renderArea.offset = vk::Offset2D{0, 0}; // TODO -3 ?

            std::array<vk::ClearValue, 2> clearValues = {};
            clearValues[0].color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            picker.commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

            picker.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, picker.pipeline.graphicsPipeline);

            vk::Rect2D scissor = {
                // vk::Offset2D {target.x, target.y},
                vk::Offset2D {0, 0},
                // vk::Extent2D {3, 3}
                extent
            };

            // picker.commandBuffer.setViewport(0, viewport);
            picker.commandBuffer.setScissor(0, scissor);

            for (auto model : models)
            {
                picker.commandBuffer.bindVertexBuffers(0, model.mesh.vertexBuffer.buffer, vk::DeviceSize{0});
                picker.commandBuffer.bindIndexBuffer(model.mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
                picker.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, picker.pipeline.layout, 0, model.colorDescriptorSet, nullptr);
                picker.commandBuffer.drawIndexed(model.mesh.indices.size(), 1, 0, 0, 0);
            }

            picker.commandBuffer.endRenderPass();
            picker.commandBuffer.end();

            // TODO: Pack the submit with the others. 
            vk::SubmitInfo submitInfo;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &picker.commandBuffer;
            
            device->graphicsQueue.submit(submitInfo, picker.renderFinished);
        }

        glm::vec3 pickColor(std::vector<SceneObject> models, glm::vec2 target)
        {
            // TODO: Call that elsewhere
            renderObjectPickerImage(models, target);

            device->logicalDevice.waitForFences(picker.renderFinished, VK_TRUE, UINT64_MAX);
            device->logicalDevice.resetFences(picker.renderFinished);

            core::Image stagingImage = core::Image(device, picker.width, picker.height, 1, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eTransferDst, 
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent); // TODO: OPTIMIZE We don't need a view
            
            // TODO: OPTIMIZE Use a single command buffer to do all necessary operations
            picker.image.transitionLayout(context, vk::ImageLayout::eTransferSrcOptimal);
            stagingImage.transitionLayout(context, vk::ImageLayout::eTransferDstOptimal);
            
            bool colorSwizzle = false;
            
            if (device->supportsBlittingToLinearImages()) {
                picker.image.blit(context, stagingImage, picker.width, picker.height);
            }
            else
            {
                picker.image.copyTo(context, stagingImage, picker.width, picker.height);
                
                // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
                // Check if source is BGR 
                // Note: Not complete, only contains most common and basic BGR surface formats for demonstation purposes
                std::vector<vk::Format> formatsBGR = { vk::Format::eB8G8R8A8Srgb, vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Snorm };
                colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), picker.image.format) != formatsBGR.end());
            }
            
            stagingImage.transitionLayout(context, vk::ImageLayout::eGeneral);
            picker.image.transitionLayout(context, vk::ImageLayout::eColorAttachmentOptimal);

            // TODO: Grab only the value in the middle pixel
            stagingImage.save("debug.ppm", colorSwizzle);
            glm::vec3 pixelValue = stagingImage.pixelAt(target.x, target.y);
            
            stagingImage.destroy();
            return pixelValue;
        }

    private:
        void createSwapchain(GLFWwindow *window)
        {
            core::SwapchainSupportDetails swapchainSupport = device->getSwapchainSupport(surface);
            vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats); // Defaults to B8G8R8A8Srgb
            vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);

            vk::Extent2D extent = chooseSwapExtent(swapchainSupport.capabilities, window);

            uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
            if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
            {
                imageCount = swapchainSupport.capabilities.maxImageCount;
            }

            vk::SwapchainCreateInfoKHR sCreateInfo;
            sCreateInfo.surface = surface;
            sCreateInfo.minImageCount = imageCount;
            sCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
            sCreateInfo.imageFormat = surfaceFormat.format;
            sCreateInfo.imageExtent = extent;
            sCreateInfo.imageArrayLayers = 1;
            sCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            sCreateInfo.presentMode = presentMode;
            sCreateInfo.clipped = VK_TRUE; // We don't care about the color of obscured pixels (ex: if another window is on top)

            core::QueueFamilyIndices indices = device->getQueueFamilyIndices();
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

            swapchain = device->logicalDevice.createSwapchainKHR(sCreateInfo);

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
            std::vector<vk::Image> imgs = device->logicalDevice.getSwapchainImagesKHR(swapchain);

            images.resize(imgs.size());

            for (size_t i = 0; i < imgs.size(); i++)
            {

                auto view = core::Image::createImageView(device, imgs[i], imageFormat, vk::ImageAspectFlagBits::eColor, 1);
                images[i] = {
                    nullptr,
                    nullptr,
                    imgs[i],
                    view,
                    vk::Fence()};
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
            return device->logicalDevice.createFramebuffer(framebufferInfo);
        }

        void setupPicker()
        {
            picker.width = 800;
            picker.height = 600;
            picker.colorImageFormat = vk::Format::eR8G8B8A8Unorm;
            createPickerRessources();
            createPickerObjectDescriptorSetLayout();
            createPickerRenderPass();
            createPickerFramebuffer();
            createPickerPipeline();
            createPickerCommandBuffer();
            createPickerFence();
        }

        void createPickerFence() {
            vk::FenceCreateInfo fenceInfo;
            picker.renderFinished = device->logicalDevice.createFence(fenceInfo);
        }

        void createPickerCommandBuffer()
        {
            picker.commandBuffer = commandPool.allocateCommandBuffers(1)[0];
        }

        void createPickerFramebuffer()
        {
            std::array<vk::ImageView, 2> attachments = {
                picker.image.view,
                picker.depthImage.view,
            };

            vk::FramebufferCreateInfo fbInfo;
            fbInfo.renderPass = picker.renderPass;
            fbInfo.attachmentCount = 2;
            fbInfo.pAttachments = attachments.data();
            fbInfo.width = picker.width;
            fbInfo.height = picker.height;
            fbInfo.layers = 1;

            picker.framebuffer = device->logicalDevice.createFramebuffer(fbInfo);
        }

        void createPickerRessources()
        {
            vk::Format dFormat = device->findDepthFormat();

            picker.depthImage = core::Image(device, picker.width, picker.height, 1, vk::SampleCountFlagBits::e1,
                                            dFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                            vk::ImageAspectFlagBits::eDepth);

            picker.image = core::Image(device, picker.width, picker.height, 1, vk::SampleCountFlagBits::e1, picker.colorImageFormat,
                                            vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                            vk::ImageAspectFlagBits::eColor);
        #ifndef NDEBUG
            device->setDebugUtilsObjectName(picker.depthImage.image, "Picker depth image");
            device->setDebugUtilsObjectName(picker.image.image, "Picker color image");
        #endif
        }

        void createPickerObjectDescriptorSetLayout()
        {
            vk::DescriptorSetLayoutBinding uboLayoutBinding;
            uboLayoutBinding.binding = 0;         // The binding used in the shader
            uboLayoutBinding.descriptorCount = 1; // Number of values in the array
            uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
            uboLayoutBinding.pImmutableSamplers = nullptr; // Image sampling related stuff.

            std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};

            vk::DescriptorSetLayoutCreateInfo createInfo{{}, (uint32_t)bindings.size(), bindings.data()};

            picker.descriptorSetLayout = device->logicalDevice.createDescriptorSetLayout(createInfo);
        }

        void createPickerRenderPass()
        {
            vk::AttachmentDescription colorAttachment;
            colorAttachment.format = picker.colorImageFormat;
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

            picker.renderPass = device->logicalDevice.createRenderPass(renderPassInfo);
        #ifndef NDEBUG
            device->setDebugUtilsObjectName(picker.renderPass, "Picker renderpass");
        #endif
        }

        void createPickerPipeline()
        {
            core::PipelineFactory factory = core::PipelineFactory(device);
            factory.setRenderPass(picker.renderPass);
            factory.setExtent(extent);
            factory.addDynamicState(vk::DynamicState::eScissor);
            factory.multisample.rasterizationSamples = vk::SampleCountFlagBits::e1; // TODO: set all of those at once.

            // TODO: Specific shaders
            factory.registerShader("shaders/picker.vert.spv", vk::ShaderStageFlagBits::eVertex);
            factory.registerShader("shaders/picker.frag.spv", vk::ShaderStageFlagBits::eFragment);
            picker.pipeline = factory.create(std::vector<vk::DescriptorSetLayout>({picker.descriptorSetLayout}));

        #ifndef NDEBUG
            device->setDebugUtilsObjectName(picker.pipeline.graphicsPipeline, "Picker graphics Pipeline");
        #endif
            // TODO: Create object picking pipeline
            //  * No actual rendering
            //  * Write vertex color to a specific buffer
            //  * Use a small viewport of around the cursor. We need to be able to specify the viewport dim when drawing
            //  *

            // 1) register simple color shaders
            // 2)
        }

        void createDepthResources()
        {
            vk::Format format = device->findDepthFormat();

            depthImage = core::Image(device, extent.width, extent.height, 1, device->msaaSamples,
                                     format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                     vk::ImageAspectFlagBits::eDepth);
        }

        void createColorResources()
        {
            colorImage = core::Image(device, extent.width, extent.height, 1, device->msaaSamples, this->imageFormat,
                                     vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                     vk::ImageAspectFlagBits::eColor);

#ifndef NDEBUG
            device->setDebugUtilsObjectName(colorImage.view, "Color Image View");
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

            objectsDescriptorSetLayout = device->logicalDevice.createDescriptorSetLayout(createInfo);

            vk::DescriptorSetLayoutBinding lightsLayoutBinding;
            lightsLayoutBinding.binding = 0;
            lightsLayoutBinding.descriptorCount = 1;
            lightsLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            lightsLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            vk::DescriptorSetLayoutCreateInfo lightsCreateInfo;
            lightsCreateInfo.bindingCount = 1;
            lightsCreateInfo.pBindings = &lightsLayoutBinding;

            lightsDescriptorSetLayout = device->logicalDevice.createDescriptorSetLayout(lightsCreateInfo);

#ifndef NDEBUG
            device->setDebugUtilsObjectName(objectsDescriptorSetLayout, "Object Descriptor Layout");
            device->setDebugUtilsObjectName(lightsDescriptorSetLayout, "Lights Descriptor Set Layout");
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

            descriptorPool = device->logicalDevice.createDescriptorPool(createInfo);
        }
    };
}; // namespace core