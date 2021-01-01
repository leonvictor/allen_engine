#pragma once

#include "core/texture_cubemap.hpp"
#include "mesh.cpp"

#include "core/context.hpp"
#include "transform.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

// TODO: Where should skyboxes live ?
//  - They're unique
//  - They're static (for now at least)
// Unity has them as an attribute of the camera
class Skybox
{
  public:
    Mesh mesh;
    core::TextureCubeMap texture;
    vk::UniqueDescriptorSet descriptorSet;
    Transform transform;
    std::shared_ptr<core::Device> device;
    std::shared_ptr<core::Context> context;

    Skybox() {}

    Skybox(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string texturePath, std::string modelPath)
    {
        this->device = device;
        this->context = context;

        texture.loadFromDirectory(context, device, texturePath);
        mesh = Mesh(device, modelPath);
        transform.scale = glm::vec3(25.0f);
    }

    // TODO: Maybe move the alloc/update to swapchain ? See mesh.cpp
    void createDescriptorSet(vk::DescriptorPool& descriptorPool, vk::DescriptorSetLayout& descriptorSetLayout)
    {

        vk::DescriptorSetAllocateInfo allocInfo{descriptorPool, 1, &descriptorSetLayout};
        descriptorSet = std::move(device->logical->allocateDescriptorSetsUnique(allocInfo)[0]);
        context->setDebugUtilsObjectName(descriptorSet.get(), "Skybox DescriptorSet");

        auto uboDescriptor = mesh.uniformBuffer.getDescriptor();
        auto cubeMapDescriptor = texture.getDescriptor();

        std::vector<vk::WriteDescriptorSet> writeDescriptors({{descriptorSet.get(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &uboDescriptor, nullptr},
                                                              {descriptorSet.get(), 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &cubeMapDescriptor, nullptr, nullptr}});

        device->logical->updateDescriptorSets(writeDescriptors, nullptr);
    }

    void updateUniformBuffer(core::UniformBufferObject ubo)
    {
        mesh.updateUniformBuffers(ubo);
    }
};