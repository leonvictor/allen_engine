#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan.hpp>

#include "graphics/device.hpp"
#include "graphics/resources/buffer.hpp"
#include "graphics/resources/texture.hpp"

#include "object_model/spatial_component.hpp"

#include "ubo.hpp"
#include "utils/files.cpp"
#include "vertex.hpp"

#include "../../material.hpp"
#include "../../mesh.hpp"

/// @brief The MeshRenderer component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class MeshRenderer : public SpatialComponent
{
  private:
    Mesh m_mesh;
    Material m_material;

    vkg::Buffer m_vertexBuffer;
    vkg::Buffer m_indexBuffer;
    vkg::Buffer m_uniformBuffer;
    vkg::Buffer m_materialBuffer;
    vkg::Texture m_materialTexture;

    std::shared_ptr<vkg::Device> m_pDevice;
    vk::UniqueDescriptorSet m_vkDescriptorSet;

    void CreateDataBuffers()
    {
        // Create vertex buffer
        vkg::Buffer vertexStagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_mesh.m_vertices);
        m_vertexBuffer = vkg::Buffer(m_pDevice, vertexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        // Create index buffer
        vkg::Buffer indexStagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_mesh.m_indices);
        m_indexBuffer = vkg::Buffer(m_pDevice, indexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        m_pDevice->GetTransferCommandPool().Execute([&](vk::CommandBuffer cb)
            {
                vertexStagingBuffer.CopyTo(cb, m_vertexBuffer);
                indexStagingBuffer.CopyTo(cb, m_indexBuffer);
            });
    }

    void CreateMaterialTexture()
    {
        // TODO: Split image loading and vulkan texture creation
        m_materialTexture = vkg::Texture(m_pDevice, m_material.m_texturePath);
    }

    void CreateUniformBuffer()
    {
        m_uniformBuffer = vkg::Buffer(m_pDevice, sizeof(vkg::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    void UpdateUniformBuffers(vkg::UniformBufferObject ubo)
    {
        m_uniformBuffer.Map(0, sizeof(ubo));
        m_uniformBuffer.Copy(&ubo, sizeof(ubo));
        m_uniformBuffer.Unmap();
    }

    /// @brief Bind the vertex and index buffers of the mesh.
    void Bind(vk::CommandBuffer& cb, vk::DeviceSize offset = 0)
    {
        // TODO: firstBinding ?
        cb.bindVertexBuffers(0, m_vertexBuffer.GetVkBuffer(), offset);
        cb.bindIndexBuffer(m_indexBuffer.GetVkBuffer(), offset, vk::IndexType::eUint32);
    }

    // TODO: Descriptor allocation and update is managed by the swapchain.
    // We could extract this part and use a method where each objects requests a descriptor from the pool ?
    // Each descriptable Component should register its descriptor to its parent object
    // *before* creation
    void CreateDescriptorSet()
    {
        m_vkDescriptorSet = m_pDevice->AllocateDescriptorSet<MeshRenderer>();
        m_pDevice->SetDebugUtilsObjectName(m_vkDescriptorSet.get(), "MeshRenderer Descriptor Set");

        std::array<vk::WriteDescriptorSet, 3> writeDescriptors = {};

        writeDescriptors[0].dstSet = m_vkDescriptorSet.get();
        writeDescriptors[0].dstBinding = 0;      // Binding index
        writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = m_uniformBuffer.GetDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        writeDescriptors[1].dstSet = m_vkDescriptorSet.get();
        writeDescriptors[1].dstBinding = 1;      // Binding index
        writeDescriptors[1].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescriptors[1].descriptorCount = 1;
        auto textureDescriptor = m_materialTexture.GetDescriptor();
        writeDescriptors[1].pImageInfo = &textureDescriptor;

        // TODO: Materials presumably don't change so they don't need a binding
        writeDescriptors[2].dstSet = m_vkDescriptorSet.get();
        writeDescriptors[2].dstBinding = 2;
        writeDescriptors[2].dstArrayElement = 0;
        writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[2].descriptorCount = 1;
        auto materialDescriptor = m_materialBuffer.GetDescriptor();
        writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?

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

    // Components methods

  public:
    /// @brief Construct a MeshRenderer component.
    /// @param pDevice: pointer to the rendering device.
    /// @param path: path to the model file.
    MeshRenderer(std::shared_ptr<vkg::Device> pDevice, std::string modelPath, std::string texturePath)
    {
        m_pDevice = pDevice;
        m_mesh.m_sourceFile = modelPath;
        m_material.m_texturePath = texturePath;
    }

    void Initialize() override
    {
        CreateDataBuffers();
        CreateUniformBuffer();
        CreateMaterialTexture();
        CreateDescriptorSet();
    }

    void Shutdown() override
    {
        m_vkDescriptorSet.reset();

        // TODO: Make sure reassignement is good enough.
        m_materialTexture = vkg::Texture();

        m_uniformBuffer = vkg::Buffer();
        m_vertexBuffer = vkg::Buffer();
        m_indexBuffer = vkg::Buffer();
    }

    bool Load() override
    {
        // Short circuit
        return m_mesh.Load() && m_material.Load();
    }

    void Unload() override
    {
        m_mesh.Unload();
        m_material.Unload();
    }
};