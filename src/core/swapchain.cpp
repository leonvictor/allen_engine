#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"
#include "context.hpp"
#include "image.cpp"
#include <memory>

#include <GLFW/glfw3.h>

namespace core {
    struct SwapchainImage {
        vk::Image image;
        vk::ImageView imageView;
        vk::Fence fence;
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

        std::shared_ptr<core::Device> device;

        Swapchain() {
            //TODO
        }
        
        void createSurface(core::Context context, GLFWwindow *window) {
            VkSurfaceKHR pSurface = VkSurfaceKHR(surface);
            if (glfwCreateWindowSurface(context.instance, window, nullptr, &pSurface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create window surface.");
            }
            surface = vk::SurfaceKHR(pSurface);
        }

        void init(core::Context context, std::shared_ptr<core::Device> device, GLFWwindow *window) {
            // TODO: Should device and context be application wide ?
            // Store a pointer to them ?
            this->device = device;

            assert(surface);
            createSwapchain(window);
            createImages();
            createDepthResources();
            createColorResources();

            initialized = true;
        } 

        void destroy() {
            //TODO
            initialized = false;
        }

        bool isInitialized() {
            return initialized;
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
                images[i] = {
                    imgs[i],
                    core::Image::createImageView(device, imgs[i], imageFormat, vk::ImageAspectFlagBits::eColor, 1),
                    nullptr //TODO: Initialize fences 
                };
            }
        }

        //TODO: WIP : requires color and depth imageviews
        // void createFramebuffers(vk::ImageView view) {
        //     std::array<vk::ImageView, 3> attachments = {
        //         colorImage.view,
        //         depthImage.view, 
        //         view
        //     };
        //     vk::FramebufferCreateInfo framebufferInfo;
        //     framebufferInfo.renderPass = renderPass; //TODO: Requires a ref to renderpass.
        //     framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        //     framebufferInfo.pAttachments = attachments.data();
        //     framebufferInfo.width = extent.width;
        //     framebufferInfo.height = extent.height;
        //     framebufferInfo.layers = 1; // Nb of layers in image array.
        //     return device->logicalDevice.createFrameBuffer(frameBufferInfo);
        // }
        
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
    };
};