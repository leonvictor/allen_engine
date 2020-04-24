#pragma once 

#include <vulkan/vulkan.hpp>
#include <array>
#include <memory>
#include "texture.cpp"

#include "device.hpp"

namespace core {

    /* Uniform buffer wrapper 
     * TODO: Move that somewhere else
    */
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection; 
    };

    class Descriptor {
    public:
        vk::DescriptorPool pool;
        vk::DescriptorSetLayout setLayout;
        std::vector<vk::DescriptorSet> sets;

        std::shared_ptr<core::Device> device;
        int nImages;

        Descriptor() {}; // TODO: Remove this when we can

        Descriptor(std::shared_ptr<core::Device> device, int nImages) {
            this->device = device;
            this->nImages = nImages;
            // createDescriptorPool(nImages);
        }

        ~Descriptor() {
            // TODO
        }

        /* TODO: The layout is used before the rest. How do we handle it ? */
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
            vk::DescriptorSetLayoutCreateInfo createInfo;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            setLayout = device->logicalDevice.createDescriptorSetLayout(createInfo);
        }

        void createDescriptorPool(int nImages) {
            // TODO: pass nImages // TODO: static cast uint32 ?
            std::array<vk::DescriptorPoolSize, 2> poolSizes;;
            poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
            poolSizes[0].descriptorCount = nImages;
            poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
            poolSizes[1].descriptorCount = nImages;

            vk::DescriptorPoolCreateInfo createInfo;
            createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            createInfo.pPoolSizes = poolSizes.data();
            createInfo.maxSets = nImages;

            pool = device->logicalDevice.createDescriptorPool(createInfo);
        }

        void createDescriptorSets(int nImages, std::vector<std::shared_ptr<core::Buffer>> uniformBuffers, const core::Texture &texture) {
            // TODO: Make sure setLayout is already initialized
            std::vector<vk::DescriptorSetLayout> layouts(nImages, setLayout);

            vk::DescriptorSetAllocateInfo allocInfo;
            allocInfo.descriptorPool = pool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(nImages);
            allocInfo.pSetLayouts = layouts.data();

            // .data() ?
            sets = device->logicalDevice.allocateDescriptorSets(allocInfo);

            for (size_t i = 0; i < nImages; i++) {
                // Describes the buffer and the region within it that contains the data for the descriptor
                vk::DescriptorBufferInfo bufferInfo;
                bufferInfo.buffer = uniformBuffers[i]->buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(core::UniformBufferObject);

                vk::DescriptorImageInfo imageInfo = {};
                imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                imageInfo.imageView = texture.image.view;
                imageInfo.sampler = texture.sampler;

                std::array<vk::WriteDescriptorSet, 2> writeDescriptors = {};
                writeDescriptors[0].dstSet = sets[i];
                writeDescriptors[0].dstBinding = 0; // Binding index 
                writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
                writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
                writeDescriptors[0].descriptorCount = 1;
                writeDescriptors[0].pBufferInfo = &bufferInfo;

                writeDescriptors[1].dstSet = sets[i];
                writeDescriptors[1].dstBinding = 1; // Binding index 
                writeDescriptors[1].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
                writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
                writeDescriptors[1].descriptorCount = 1;
                writeDescriptors[1].pImageInfo = &imageInfo;

                device->logicalDevice.updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
            }
        }
    private:
    };
}