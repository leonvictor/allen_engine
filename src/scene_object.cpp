#pragma once

#include "mesh.cpp"
#include "core/texture.cpp"
#include "material.cpp"
#include "core/device.hpp"
#include <glm/glm.hpp>
#include "transform.cpp"
#include "utils/color_uid.cpp"

class SceneObject
{
public:
    Mesh mesh;
    core::Texture texture;
    Material material;

    Transform transform;
    
    std::shared_ptr<core::Device> device;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorSet colorDescriptorSet;

    ColorUID colorId;

    SceneObject(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string path,
                glm::vec3 position = glm::vec3(0.0f), glm::vec3 color = {1.0f, 1.0f, 1.0f},
                MaterialBufferObject material = MaterialBufferObject(),
                std::string texturePath = "")
    {
        this->device = device;
        mesh = Mesh::fromObj(context, device, path, color);
        
        transform.position = position;
        
        addMaterial(material);

        if (!texturePath.empty()) {
            texture = core::Texture(context, device, texturePath);
        }
    }

    glm::mat4 getModelMatrix() {
        return glm::translate(glm::scale(modelMatrix, transform.scale), transform.position);
    }

    void addMaterial(MaterialBufferObject newMaterial)
    {
        material = Material(device, newMaterial);
    }

    // TODO: Descriptor allocation and update is managed by the swapchain.
    // We could extract this part and use a method where each objects requests a descriptor from the pool ?
    void createDescriptorSet(vk::DescriptorPool &descriptorPool, vk::DescriptorSetLayout &descriptorSetLayout)
    {
        // TODO: Make sure setLayout is already initialized
        vk::DescriptorSetAllocateInfo allocInfo{descriptorPool, 1, &descriptorSetLayout};
        descriptorSet = device->logicalDevice.allocateDescriptorSets(allocInfo)[0];

        std::array<vk::WriteDescriptorSet, 3> writeDescriptors = {};

        writeDescriptors[0].dstSet = descriptorSet;
        writeDescriptors[0].dstBinding = 0;      // Binding index
        writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = mesh.uniformBuffer.getDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        writeDescriptors[1].dstSet = descriptorSet;
        writeDescriptors[1].dstBinding = 1;      // Binding index
        writeDescriptors[1].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescriptors[1].descriptorCount = 1;
        auto textureDescriptor = texture.getDescriptor();
        writeDescriptors[1].pImageInfo = &textureDescriptor;

        // TODO: Materials presumably don't change so they don't need a binding
        writeDescriptors[2].dstSet = descriptorSet;
        writeDescriptors[2].dstBinding = 2;
        writeDescriptors[2].dstArrayElement = 0;
        writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[2].descriptorCount = 1;
        auto materialDescriptor = material.getBufferDescriptor();
        writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?

        device->logicalDevice.updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }

    void createColorDescriptorSet(vk::DescriptorPool &descriptorPool, vk::DescriptorSetLayout &descriptorSetLayout)
    {
        vk::DescriptorSetAllocateInfo allocInfo{descriptorPool, 1, &descriptorSetLayout};
        colorDescriptorSet = device->logicalDevice.allocateDescriptorSets(allocInfo)[0];

        std::array<vk::WriteDescriptorSet, 1> writeDescriptors = {};

        writeDescriptors[0].dstSet = colorDescriptorSet;
        writeDescriptors[0].dstBinding = 0;      // Binding index
        writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = mesh.uniformBuffer.getDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        device->logicalDevice.updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }

    void destroy()
    {
        mesh.destroy();
        texture.destroy();
        material.destroy();
    }
private:
    glm::mat4 modelMatrix = glm::mat4(1.0f); // TODO
};