#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"
#include "context.hpp"
#include "image.cpp"
#include "commandpool.cpp"
#include "pipeline.cpp"
#include "buffer.cpp"
#include <memory>

#include <array>
#include <GLFW/glfw3.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace core {
    struct SwapchainImage {
        vk::Framebuffer framebuffer;
        vk::CommandBuffer commandbuffer;
        vk::Image image;
        vk::ImageView imageView;
        vk::Fence fence;
        
        void cleanup(vk::Device& device, const vk::CommandPool& commandPool) {
            device.destroyFramebuffer(framebuffer);
            device.freeCommandBuffers(commandPool, commandbuffer);
            device.destroyImageView(imageView);
        }
    };

    class Swapchain {
    public:
        core::Image colorImage;
        core::Image depthImage;

        core::Pipeline graphicsPipeline;
        vk::SurfaceKHR surface;
        vk::SwapchainKHR swapchain;
        vk::PresentInfoKHR presentInfo;

        std::vector<SwapchainImage> images;
        
        vk::Format imageFormat;
        vk::Extent2D extent;
        vk::RenderPass renderPass;

        std::shared_ptr<core::Device> device;
        core::CommandPool commandPool;

        std::vector<vk::Semaphore> imageAvailableSemaphores;
        std::vector<vk::Semaphore> renderFinishedSemaphores;
        std::vector<vk::Fence> inFlightFences;

        int currentFrame = 0;
        bool frame_active = false;
        uint32_t activeFrameIndex;

        vk::DescriptorSetLayout descriptorSetLayout;
        vk::DescriptorPool descriptorPool;

        Swapchain() {}
        
        void createSurface(std::shared_ptr<core::Context> context, GLFWwindow *window) {
            VkSurfaceKHR pSurface = VkSurfaceKHR(surface);
            if (glfwCreateWindowSurface(context->instance.get(), window, nullptr, &pSurface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create window surface.");
            }
            surface = vk::SurfaceKHR(pSurface);
        }

        void recreate(GLFWwindow *window) {
            createSwapchain(window);
            createImages();
            createRenderPass();
            graphicsPipeline.createGraphicsPipeline(device, extent, descriptorSetLayout, renderPass);
            createDepthResources();
            createColorResources();
            createFramebuffers();
            createDescriptorPool(images.size());

            // TODO : Add descriptor sets and command buffers here
        }

        // TODO: Normalize object construction
        void init(std::shared_ptr<core::Device> device, GLFWwindow *window) {
            this->device = device;

            assert(surface);
            createSwapchain(window);
            createImages();
            createDepthResources();
            createColorResources();
            createRenderPass();
            createDescriptorSetLayout();
            graphicsPipeline.createGraphicsPipeline(device, extent, descriptorSetLayout, renderPass);
            createFramebuffers();
            createSyncObjects();
            createDescriptorPool(images.size());

        }

        void createFramebuffers() {  
            for (int i = 0; i < images.size(); i++) {
                images[i].framebuffer = createFramebuffer(images[i].imageView, renderPass);
            }
        }

        void createRenderPass() {
            assert(device);

            vk::AttachmentDescription colorAttachment;
            colorAttachment.format = imageFormat;
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

        void createCommandBuffers(const core::CommandPool& commandPool, std::vector<Mesh> models) {

            this->commandPool = commandPool;
            auto commandBuffers = commandPool.allocateCommandBuffers(images.size());

            for (size_t i = 0; i < images.size(); i++) {
                commandBuffers[i].begin(vk::CommandBufferBeginInfo{});

                // Start a render pass.
                vk::RenderPassBeginInfo renderPassInfo;
                renderPassInfo.renderPass = renderPass;
                renderPassInfo.framebuffer = images[i].framebuffer;
                renderPassInfo.renderArea.extent = extent;
                renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};

                std::array<vk::ClearValue, 2> clearValues = {};
                clearValues[0].color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
                clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
                renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                renderPassInfo.pClearValues = clearValues.data();

                commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

                /* Draw command buffer */
                commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.graphicsPipeline);
                
                for (auto model : models) {
                    commandBuffers[i].bindVertexBuffers(0, model.vertexBuffer.buffer , vk::DeviceSize{0});
                    commandBuffers[i].bindIndexBuffer(model.indexBuffer.buffer, 0, vk::IndexType::eUint32);
                    commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline.layout, 0, 1, &model.descriptorSet, 0, nullptr);
                    commandBuffers[i].drawIndexed(model.indices.size(), 1, 0, 0, 0);
                }
                commandBuffers[i].endRenderPass();
                commandBuffers[i].end();

                images[i].commandbuffer = commandBuffers[i];
            }
        }

        void createSyncObjects() {
            imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
            
            vk::SemaphoreCreateInfo semaphoreInfo;

            vk::FenceCreateInfo fenceInfo;
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled; // Create fences in the signaled state
            
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                imageAvailableSemaphores[i] = device->logicalDevice.createSemaphore(semaphoreInfo, nullptr);
                renderFinishedSemaphores[i] = device->logicalDevice.createSemaphore(semaphoreInfo, nullptr);
                inFlightFences[i] = device->logicalDevice.createFence(fenceInfo, nullptr);
            }
        }

        /* Destroy the parts we need to recreate */
        void cleanup() {
            colorImage.destroy();
            depthImage.destroy();

            for (auto img : images) {
                img.cleanup(device->logicalDevice, commandPool.pool);
            }

            graphicsPipeline.destroy();
            device->logicalDevice.destroyRenderPass(renderPass);

            device->logicalDevice.destroySwapchainKHR(swapchain);
        
            device->logicalDevice.destroyDescriptorPool(descriptorPool);
        }

        void destroy() {
            cleanup();
            
            device->logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                device->logicalDevice.destroySemaphore(imageAvailableSemaphores[i]);
                device->logicalDevice.destroySemaphore(renderFinishedSemaphores[i]);
                device->logicalDevice.destroyFence(inFlightFences[i]);
            }
        }

    private:

        void retrievePersistentContextInfo() {

        }

        void createSwapchain(GLFWwindow *window) {

            /* Swapchain parameters */
            core::SwapchainSupportDetails swapchainSupport = device->getSwapchainSupport(surface);
            vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
            vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
            
            vk::Extent2D extent = chooseSwapExtent(swapchainSupport.capabilities, window);

            uint32_t imageCount = swapchainSupport.capabilities.minImageCount +1;
            if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
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

            if (indices.graphicsFamily != indices.presentFamily) {
                sCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
                sCreateInfo.queueFamilyIndexCount = 2;
                sCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                sCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
                sCreateInfo.queueFamilyIndexCount = 0; // Optionnal
                sCreateInfo.pQueueFamilyIndices = nullptr; // Optionnal
            }

            sCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;
            sCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            sCreateInfo.oldSwapchain = {(VkSwapchainKHR_T *)0}; // TODO : Pass the old swapchain to reuse most of the info.

            swapchain = device->logicalDevice.createSwapchainKHR(sCreateInfo);

            this->extent = extent;
            imageFormat = surfaceFormat.format;
        }

        vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
            for (const auto& format : availableFormats){
                // Return the first format that supports SRGB color space in 8bit by channels
                if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && format.format == vk::Format::eB8G8R8A8Srgb) {
                    return format;
                }
            }
            return availableFormats[0];
        }

        vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availableModes) {
            for (const auto& mode : availableModes) {
                if (mode == vk::PresentModeKHR::eMailbox) {
                    return mode;
                }
            }
            return vk::PresentModeKHR::eFifo;
        }

        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow *window) {
            if (capabilities.currentExtent.width != UINT32_MAX) {
                return capabilities.currentExtent;
            } else {
                // Some window managers do not specify the resolution (indicated by special max value)
                // In this case, use the resolution that best matches the window within the ImageExtent bounds
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D extent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                    };

                extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                return extent;
            }
        }

        void createImages() {
            std::vector<vk::Image> imgs = device->logicalDevice.getSwapchainImagesKHR(swapchain);

            images.resize(imgs.size());

            for (size_t i = 0; i < imgs.size(); i++) {

                auto view = core::Image::createImageView(device, imgs[i], imageFormat, vk::ImageAspectFlagBits::eColor, 1);
                images[i] = {
                    nullptr,
                    nullptr,
                    imgs[i],
                    view,
                    vk::Fence()
                };
            }
        }

        vk::Framebuffer createFramebuffer(vk::ImageView view, vk::RenderPass renderPass) {
            std::array<vk::ImageView, 3> attachments = {
                colorImage.view,
                depthImage.view, 
                view
            };
            vk::FramebufferCreateInfo framebufferInfo;
            framebufferInfo.renderPass = renderPass; //TODO: Requires a ref to renderpass.
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1; // Nb of layers in image array.
            return device->logicalDevice.createFramebuffer(framebufferInfo);
        }
        
        void createDepthResources() {
            vk::Format format = device->findDepthFormat();

            depthImage = core::Image(device, extent.width, extent.height, 1, device->msaaSamples, 
                format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                vk::ImageAspectFlagBits::eDepth);
        }

        void createColorResources() {
            colorImage = core::Image(device, extent.width, extent.height, 1, device->msaaSamples, this->imageFormat,
                vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment| vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                vk::ImageAspectFlagBits::eColor);

            device->setDebugUtilsObjectName(colorImage.view.objectType, (uint64_t) (VkImageView) colorImage.view, "Color Image view");
        }

        void createDescriptorSetLayout() {
            vk::DescriptorSetLayoutBinding uboLayoutBinding;
            uboLayoutBinding.binding = 0; // The binding used in the shader
            uboLayoutBinding.descriptorCount = 1; // Number of values in the array
            uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex; // We're only referencing the descriptor from the vertex shader
            uboLayoutBinding.pImmutableSamplers = nullptr; // Image sampling related stuff.

            vk::DescriptorSetLayoutBinding samplerLayoutBinding;
            samplerLayoutBinding.binding = 1;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            samplerLayoutBinding.pImmutableSamplers = nullptr;
            samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment; //It's possible to use texture sampling in the vertex shader as well, for example to dynamically deform a grid of vertices by a heightmap

            std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
            
            vk::DescriptorSetLayoutCreateInfo createInfo{ {}, (uint32_t) bindings.size(), bindings.data() };

            descriptorSetLayout = device->logicalDevice.createDescriptorSetLayout(createInfo);
        }

        void createDescriptorPool(int nObjects) {
            std::array<vk::DescriptorPoolSize, 2> poolSizes;;
            poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
            poolSizes[0].descriptorCount = nObjects;
            poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
            poolSizes[1].descriptorCount = nObjects;

            vk::DescriptorPoolCreateInfo createInfo;
            createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            createInfo.pPoolSizes = poolSizes.data();
            createInfo.maxSets = nObjects;

            descriptorPool = device->logicalDevice.createDescriptorPool(createInfo);
        }
    };
};