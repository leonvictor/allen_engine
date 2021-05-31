#pragma once

#include "components.hpp"

#include "graphics/device.hpp"
#include "graphics/resources/texture.hpp"
#include "material.cpp"
#include "mesh.cpp"
#include "transform.hpp"
#include "utils/color_uid.cpp"
#include <glm/glm.hpp>

// TODO: Refactor code style

class SceneObject : public Entity
{
  public:
    Material material;

    std::shared_ptr<vkg::Device> m_pDevice;

    vk::UniqueDescriptorSet descriptorSet;
    vk::UniqueDescriptorSet colorDescriptorSet;

    ColorUID colorId;

    // TODO: Refactor to use composition
    SceneObject(std::shared_ptr<vkg::Device> pDevice, std::string modelPath,
                glm::vec3 position = glm::vec3(0.0f),
                MaterialBufferObject material = MaterialBufferObject(),
                std::string texturePath = "")
    {
        m_pDevice = pDevice;

        colorId.generate();

        addComponent<Mesh>(std::make_shared<Mesh>(m_pDevice, modelPath, colorId.toRGB()));

        std::shared_ptr<Transform> transform = std::make_shared<Transform>();
        transform->position = position;
        addComponent<Transform>(transform);

        // TODO: Material is a component.
        // ... But for now everything about them is poopy
        addMaterial(material);

        if (!texturePath.empty())
        {
            addComponent<vkg::Texture>(std::make_shared<vkg::Texture>(m_pDevice, texturePath));
        }
        createDescriptorSet();
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
        material = Material(m_pDevice, newMaterial);
    }

    // TODO: Descriptor allocation and update is managed by the swapchain.
    // We could extract this part and use a method where each objects requests a descriptor from the pool ?
    // Each descriptable Component should register its descriptor to its parent object
    // *before* creation
    void createDescriptorSet()
    {
        auto mesh = getComponent<Mesh>();

        descriptorSet = m_pDevice->AllocateDescriptorSet<SceneObject>();
        m_pDevice->SetDebugUtilsObjectName(descriptorSet.get(), "SceneObject Descriptor Set");

        std::array<vk::WriteDescriptorSet, 3> writeDescriptors = {};

        writeDescriptors[0].dstSet = descriptorSet.get();
        writeDescriptors[0].dstBinding = 0;      // Binding index
        writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = mesh->uniformBuffer.GetDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        writeDescriptors[1].dstSet = descriptorSet.get();
        writeDescriptors[1].dstBinding = 1;      // Binding index
        writeDescriptors[1].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescriptors[1].descriptorCount = 1;
        auto textureDescriptor = getComponent<vkg::Texture>()->GetDescriptor();
        writeDescriptors[1].pImageInfo = &textureDescriptor;

        // TODO: Materials presumably don't change so they don't need a binding
        writeDescriptors[2].dstSet = descriptorSet.get();
        writeDescriptors[2].dstBinding = 2;
        writeDescriptors[2].dstArrayElement = 0;
        writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[2].descriptorCount = 1;
        auto materialDescriptor = material.getBufferDescriptor();
        writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?

        m_pDevice->GetVkDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }

    void createColorDescriptorSet(vk::DescriptorPool& descriptorPool, vk::DescriptorSetLayout& descriptorSetLayout)
    {
        auto mesh = getComponent<Mesh>();

        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = descriptorPool,
        allocInfo.descriptorSetCount = 1,
        allocInfo.pSetLayouts = &descriptorSetLayout,

        colorDescriptorSet = std::move(m_pDevice->GetVkDevice().allocateDescriptorSetsUnique(allocInfo)[0]);

        std::array<vk::WriteDescriptorSet, 1> writeDescriptors = {};

        writeDescriptors[0].dstSet = colorDescriptorSet.get();
        writeDescriptors[0].dstBinding = 0;      // Binding index
        writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = mesh->uniformBuffer.GetDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        m_pDevice->GetVkDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }

    /// @brief Returns the vulkan bindings representing a scene object.
    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{
            {
                // UBO
                .binding = 0, // The binding used in the shader
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1, // Number of values in the array
                // We need to access the ubo in the fragment shader aswell now (because it contains light direction)
                // TODO: There is probably a cleaner way (a descriptor for all light sources for example ?)
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                .pImmutableSamplers = nullptr, // Image sampling related stuff.
            },
            {
                // Sampler
                .binding = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment, //It's possible to use texture sampling in the vertex shader as well, for example to dynamically deform a grid of vertices by a heightmap
                .pImmutableSamplers = nullptr,
            },
            {
                // Material
                .binding = 2,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            }};
        return bindings;
    }

    vk::DescriptorSet& GetDescriptorSet() { return descriptorSet.get(); }
};