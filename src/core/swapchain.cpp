#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"
#include "context.hpp"
#include "image.cpp"
#include "commandpool.cpp"
#include "pipeline.cpp"
#include "descriptor.cpp"
#include "buffer.cpp"
#include <memory>

#include <array>
#include <GLFW/glfw3.h>

namespace core {
    struct SwapchainImage {
        vk::Image image;
        vk::ImageView imageView;
        vk::Fence fence;
        vk::Framebuffer framebuffer;
        vk::CommandBuffer commandbuffer;
    };

    class Swapchain {
    public:
        vk::SurfaceKHR surface;
        vk::SwapchainKHR swapchain;
        vk::PresentInfoKHR presentInfo;

        std::vector<SwapchainImage> images;
        
        vk::Format imageFormat;
        vk::Extent2D extent;

        core::Image depthImage;
        core::Image colorImage;

        vk::RenderPass renderPass;

        std::shared_ptr<core::Device> device;

        Swapchain() {
            //TODO
        }
        
        void createSurface(std::shared_ptr<core::Context> context, GLFWwindow *window) {
            VkSurfaceKHR pSurface = VkSurfaceKHR(surface);
            if (glfwCreateWindowSurface(context->instance.get(), window, nullptr, &pSurface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create window surface.");
            }
            surface = vk::SurfaceKHR(pSurface);
        }

        // TODO: const context ?
        void init(std::shared_ptr<core::Device> device, GLFWwindow *window) {
            // TODO: Should device and context be application wide ?
            // Store a pointer to them ?
            this->device = device;

            assert(surface);
            createSwapchain(window);
            createImages();
            createDepthResources();
            createColorResources();
            createRenderPass();

            initialized = true;
        }

        void initFramebuffers(vk::RenderPass renderpass) {  
            for (int i = 0; i < images.size(); i++) {
                images[i].framebuffer = createFramebuffer(images[i].imageView, renderpass);;
            }
        }

        void destroy() {
            //TODO
            initialized = false;
        }

        bool isInitialized() {
            return initialized;
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

    private:
        bool initialized = false;

        void createSwapchain(GLFWwindow *window) {
            core::SwapchainSupportDetails swapchainSupport = core::querySwapchainSupport(device->physicalDevice, surface);
            vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
            vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
            vk::Extent2D extent = chooseSwapExtent(swapchainSupport.capabilities, window);

            uint32_t imageCount = swapchainSupport.capabilities.minImageCount +1;
            if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
                imageCount = swapchainSupport.capabilities.maxImageCount;
            }

            vk::SwapchainCreateInfoKHR sCreateInfo;
            sCreateInfo.surface = surface; // TODO: Should surface be already created ? I think so
            sCreateInfo.minImageCount = imageCount;
            sCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
            sCreateInfo.imageFormat = surfaceFormat.format;
            sCreateInfo.imageExtent = extent;
            sCreateInfo.imageArrayLayers = 1;
            sCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            sCreateInfo.presentMode = presentMode; // TODO: We're juste casting for now.
            sCreateInfo.clipped = VK_TRUE; // We don't care about the color of obscured pixels (ex: if another window is on top)
            
            core::QueueFamilyIndices indices = core::findQueueFamilies(device->physicalDevice, surface);
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
                    imgs[i],
                    view,
                    nullptr, //TODO: Initialize fences
                    nullptr // Framebuffers are initialized after the renderpass TODO: This is bad practice
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
        }

        void createCommandBuffers(const core::CommandPool& commandPool, const core::Pipeline& pipeline,
                core::Buffer& vertexBuffer, core::Buffer& indexBuffer, // TODO: Move this out. Maybe from a model ?
                const core::Descriptor& descriptor, int nIndices) {

            auto commandBuffers = commandPool.allocateCommandBuffers(images.size());

            for (size_t i = 0; i < images.size(); i++) {
                commandBuffers[i].begin(vk::CommandBufferBeginInfo{});

                // Start a render pass. TODO: Why is that here ?
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
                commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.graphicsPipeline);
                
                vk::Buffer vertexBuffers[] = {vertexBuffer.buffer}; // TODO
                vk::DeviceSize offsets[] = {0};
                
                commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);
                commandBuffers[i].bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint32);
                commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, 1, &descriptor.sets[i], 0, nullptr);

                commandBuffers[i].drawIndexed(nIndices, 1, 0, 0, 0);
                commandBuffers[i].endRenderPass();
                commandBuffers[i].end();

                images[i].commandbuffer = commandBuffers[i];
            }
        }
    };
};