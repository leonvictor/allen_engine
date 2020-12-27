#pragma once

#include "components.hpp"
#include "core/device.hpp"
#include "core/texture.hpp"
#include "material.cpp"
#include "mesh.cpp"
#include "transform.hpp"
#include "utils/color_uid.cpp"
#include <glm/glm.hpp>

class SceneObject : public Entity
{
  public:
    Material material;

    std::shared_ptr<core::Device> device;
    std::shared_ptr<core::Context> context;

    vk::UniqueDescriptorSet descriptorSet;
    vk::UniqueDescriptorSet colorDescriptorSet;

    ColorUID colorId;

    // TODO: Refactor to use composition
    SceneObject(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string modelPath,
                glm::vec3 position = glm::vec3(0.0f),
                MaterialBufferObject material = MaterialBufferObject(),
                std::string texturePath = "")
    {
        this->device = device;
        this->context = context;

        colorId.generate();

        addComponent<Mesh>(std::make_shared<Mesh>(device, modelPath, colorId.toRGB()));

        std::shared_ptr<Transform> transform = std::make_shared<Transform>();
        transform->position = position;
        addComponent<Transform>(transform);

        // TODO: Material is a component.
        // ... But for now everything about them is poopy
        addMaterial(material);

        if (!texturePath.empty())
        {
            addComponent<core::Texture>(std::make_shared<core::Texture>(context, texturePath));
        }
    }

    glm::mat4 getModelMatrix()
    {
        // TODO: This is a system
        std::shared_ptr<Transform> transform = getComponent<Transform>();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, transform->position);
        model = glm::rotate(model, glm::radians(transform->rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(transform->rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(transform->rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, transform->scale);
        return model;
    }

    void addMaterial(MaterialBufferObject newMaterial)
    {
        material = Material(device, newMaterial);
    }

    // TODO: Descriptor allocation and update is managed by the swapchain.
    // We could extract this part and use a method where each objects requests a descriptor from the pool ?
    // Each descriptable Component should register its descriptor to its parent object
    // *before* creation
    void createDescriptorSet(vk::DescriptorPool& descriptorPool, vk::DescriptorSetLayout& descriptorSetLayout)
    {
        auto mesh = getComponent<Mesh>();

        // TODO: Make sure setLayout is already initialized
        vk::DescriptorSetAllocateInfo allocInfo{descriptorPool, 1, &descriptorSetLayout};
        descriptorSet = std::move(device->logical.get().allocateDescriptorSetsUnique(allocInfo)[0]);
        context->setDebugUtilsObjectName(descriptorSet.get(), "SceneObject Descriptor Set");

        std::array<vk::WriteDescriptorSet, 3> writeDescriptors = {};

        writeDescriptors[0].dstSet = descriptorSet.get();
        writeDescriptors[0].dstBinding = 0;      // Binding index
        writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = mesh->uniformBuffer.getDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        writeDescriptors[1].dstSet = descriptorSet.get();
        writeDescriptors[1].dstBinding = 1;      // Binding index
        writeDescriptors[1].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescriptors[1].descriptorCount = 1;
        auto textureDescriptor = getComponent<core::Texture>()->getDescriptor();
        writeDescriptors[1].pImageInfo = &textureDescriptor;

        // TODO: Materials presumably don't change so they don't need a binding
        writeDescriptors[2].dstSet = descriptorSet.get();
        writeDescriptors[2].dstBinding = 2;
        writeDescriptors[2].dstArrayElement = 0;
        writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[2].descriptorCount = 1;
        auto materialDescriptor = material.getBufferDescriptor();
        writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?

        device->logical.get().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }

    void createColorDescriptorSet(vk::DescriptorPool& descriptorPool, vk::DescriptorSetLayout& descriptorSetLayout)
    {
        auto mesh = getComponent<Mesh>();

        vk::DescriptorSetAllocateInfo allocInfo{descriptorPool, 1, &descriptorSetLayout};
        colorDescriptorSet = std::move(device->logical.get().allocateDescriptorSetsUnique(allocInfo)[0]);

        std::array<vk::WriteDescriptorSet, 1> writeDescriptors = {};

        writeDescriptors[0].dstSet = colorDescriptorSet.get();
        writeDescriptors[0].dstBinding = 0;      // Binding index
        writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = mesh->uniformBuffer.getDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        device->logical.get().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }
};