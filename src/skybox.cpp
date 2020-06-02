#pragma once

#include "mesh.cpp"
#include "core/textureCubeMap.cpp"

#include <vulkan/vulkan.hpp>
#include <memory>
#include "transform.cpp"

class Skybox {
    public:
    Mesh mesh;
    core::TextureCubeMap texture;
    vk::DescriptorSet descriptorSet;
    Transform transform;
    std::shared_ptr<core::Device> device;

    Skybox() {}
    
    Skybox(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string texturePath, std::string modelPath) {
        this->device = device;

        texture.loadFromDirectory(context, device, texturePath);
        mesh = Mesh::fromObj(context, device, modelPath);
        transform.scale = glm::vec3(25.0f);
    }

    // TODO: Maybe move the alloc/update to swapchain ? See mesh.cpp
    void createDescriptorSet(vk::DescriptorPool& descriptorPool, vk::DescriptorSetLayout& descriptorSetLayout) {

            vk::DescriptorSetAllocateInfo allocInfo{ descriptorPool, 1, &descriptorSetLayout };
            descriptorSet = device->logicalDevice.allocateDescriptorSets(allocInfo)[0];

            auto uboDescriptor = mesh.uniformBuffer.getDescriptor();
            auto cubeMapDescriptor = texture.getDescriptor();

            std::vector<vk::WriteDescriptorSet> writeDescriptors({
                { descriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &uboDescriptor, nullptr },
                { descriptorSet, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler,  &cubeMapDescriptor, nullptr, nullptr }
            });

            device->logicalDevice.updateDescriptorSets(writeDescriptors, nullptr);
    }

    void updateUniformBuffer(core::UniformBufferObject ubo) {
        mesh.updateUniformBuffers(ubo);
    }

    void destroy() {
        mesh.destroy();
        texture.destroy();
    }
};