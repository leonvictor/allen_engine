#pragma once

#include "graphics/device.hpp"
#include "graphics/resources/texture_cubemap.hpp"
#include "mesh.cpp"

#include "transform.hpp"
#include <memory>

#include <vulkan/vulkan.hpp>

// TODO: Update to new syntax

// TODO: Where should skyboxes live ?
//  - They're unique
//  - They're static (for now at least)
// Unity has them as an attribute of the camera
class Skybox
{
  public:
    Mesh mesh;
    vkg::TextureCubeMap texture;
    vk::UniqueDescriptorSet descriptorSet;
    Transform transform;
    std::shared_ptr<vkg::Device> m_pDevice;

    Skybox() {}

    Skybox(std::shared_ptr<vkg::Device> pDevice, std::string texturePath, std::string modelPath)
    {
        m_pDevice = pDevice;

        texture.LoadFromDirectory(m_pDevice, texturePath);
        mesh = Mesh(m_pDevice, modelPath);
        transform.scale = glm::vec3(25.0f);
        createDescriptorSet();
    }

    // TODO: Maybe move the alloc/update to swapchain ? See mesh.cpp
    void createDescriptorSet()
    {

        descriptorSet = m_pDevice->AllocateDescriptorSet<Skybox>();
        m_pDevice->SetDebugUtilsObjectName(descriptorSet.get(), "Skybox Descriptor Set");

        auto uboDescriptor = mesh.uniformBuffer.GetDescriptor();
        auto cubeMapDescriptor = texture.GetDescriptor();

        std::vector<vk::WriteDescriptorSet> writeDescriptors = {
            {
                .dstSet = descriptorSet.get(),
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pImageInfo = nullptr,
                .pBufferInfo = &uboDescriptor,
                .pTexelBufferView = nullptr,
            },
            {
                .dstSet = descriptorSet.get(),
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &cubeMapDescriptor,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr,
            },
        };

        m_pDevice->GetVkDevice().updateDescriptorSets(writeDescriptors, nullptr);
    }

    void updateUniformBuffer(vkg::UniformBufferObject ubo)
    {
        mesh.updateUniformBuffers(ubo);
    }

    /// @brief Returns the vulkan bindings representing a skybox.
    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{
            {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
            {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}};

        return bindings;
    }

    vk::DescriptorSet& GetDescriptorSet() { return descriptorSet.get(); }
};