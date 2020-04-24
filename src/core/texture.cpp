#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include "image.cpp"
#include "device.hpp"
#include "buffer.cpp"
#include "context.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace core {
    // TODO: 2D and 3D
    /* TODO: Texture is just an image with a sampler on top. Simplify it */
    class Texture {
    public:
        core::Image image;
        vk::Sampler sampler; // TODO: Does each image have a sampler or do they share it ?
        int mipLevels;
        std::shared_ptr<core::Device> device;
        std::shared_ptr<core::Context> context;

        Texture(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string path) {
            this->context = context;
            this->device = device;

            createTextureImage(context, device, path);
            createSampler();
        }

    private:
        void createTextureImage(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string path) {
            
            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if (!pixels){
                throw std::runtime_error("Failed to load texture image.");
            }

            vk::DeviceSize imageSize = texWidth * texHeight * 4;
            mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
            
            core::Buffer stagingBuffer(device, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            stagingBuffer.map(0, imageSize);
            stagingBuffer.copy(pixels, static_cast<size_t>(imageSize));
            stagingBuffer.unmap();

            image = core::Image(device, texWidth, texHeight, mipLevels, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

            image.transitionLayout(context, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
            context->copyBufferToImage(stagingBuffer, image, texWidth, texHeight);
            generateMipMaps(image, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);

            image.initView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);

            stagingBuffer.cleanup();
        }

        /* TODO :
         * - Convert to vk.hpp
         * - Move to logical place
         */
        void generateMipMaps(vk::Image image, vk::Format format, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels) {

            auto formatProperties = device->physicalDevice.getFormatProperties(format);
            auto commandBuffers = context->graphicsCommandPool.beginSingleTimeCommands();

            vk::ImageMemoryBarrier barrier;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = texWidth;
            int32_t mipHeight = texHeight;
            
            for (uint32_t i = 1; i < mipLevels; i++) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal; // TODO: DST or SRC ?
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

                commandBuffers[0].pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, 
                    vk::DependencyFlags(),
                    nullptr,
                    nullptr,
                    barrier
                );

                vk::ImageBlit blit;
                blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
                blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1}; // Where is the data that we will blit from ?
                blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.layerCount = 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
                blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.layerCount = 1;
                blit.dstSubresource.baseArrayLayer = 0;

                commandBuffers[0].blitImage(
                    image, vk::ImageLayout::eTransferSrcOptimal,
                    image, vk::ImageLayout::eTransferDstOptimal,
                    blit, vk::Filter::eLinear);

                barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
                barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

                commandBuffers[0].pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                    vk::DependencyFlags(),
                    nullptr,
                    nullptr,
                    barrier
                );
                
                // Handle cases where the image is not square
                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal; // TODO: dst or src ?
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            
            // Transition the last mip level to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            commandBuffers[0].pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                vk::DependencyFlags(),
                nullptr,
                nullptr,
                barrier
            );
            
            context->graphicsCommandPool.endSingleTimeCommands(commandBuffers, device->graphicsQueue);
        }
        
        void createSampler() {
            vk::SamplerCreateInfo samplerInfo;
            samplerInfo.magFilter = vk::Filter::eLinear; // How to interpolate texels that are magnified...
            samplerInfo.minFilter = vk::Filter::eLinear; // or minified
            // Addressing mode per axis
            samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat; // x
            samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat; // y
            samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat; // z
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16;
            samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = vk::CompareOp::eAlways;
            samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
            samplerInfo.mipLodBias = 0;
            samplerInfo.maxLod = static_cast<uint32_t>(mipLevels);
            samplerInfo.minLod = 0;

            sampler = device->logicalDevice.createSampler(samplerInfo);
        }
    };
}