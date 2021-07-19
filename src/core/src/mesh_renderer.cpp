#include "mesh_renderer.hpp"

#include <graphics/device.hpp>

namespace aln
{

void MeshRenderer::CreateDataBuffers()
{
    // Create vertex buffer
    vkg::resources::Buffer vertexStagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_mesh.m_vertices);
    m_vertexBuffer = vkg::resources::Buffer(m_pDevice, vertexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Create index buffer
    vkg::resources::Buffer indexStagingBuffer(m_pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_mesh.m_indices);
    m_indexBuffer = vkg::resources::Buffer(m_pDevice, indexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

    m_pDevice->GetTransferCommandPool().Execute([&](vk::CommandBuffer cb)
        {
            vertexStagingBuffer.CopyTo(cb, m_vertexBuffer);
            indexStagingBuffer.CopyTo(cb, m_indexBuffer);
        });
}

void MeshRenderer::CreateMaterialTexture()
{
    // TODO: Split image loading and vulkan texture creation
    m_materialTexture = vkg::resources::Image::FromFile(m_pDevice, m_material.m_texturePath);
    m_materialTexture.AddView();
    m_materialTexture.AddSampler();
    m_materialTexture.SetDebugName("Material Texture");

    m_materialBuffer = vkg::resources::Buffer(m_pDevice, sizeof(MaterialBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

    // TMP while materials are poopy
    auto material = MaterialBufferObject();
    m_materialBuffer.Map(0, sizeof(material));
    m_materialBuffer.Copy(&material, sizeof(material));
    m_materialBuffer.Unmap();
}

void MeshRenderer::CreateUniformBuffer()
{
    m_uniformBuffer = vkg::resources::Buffer(m_pDevice, sizeof(vkg::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

// TODO: Descriptor allocation and update is managed by the swapchain.
// We could extract this part and use a method where each objects requests a descriptor from the pool ?
// Each descriptable Component should register its descriptor to its parent object
// *before* creation
void MeshRenderer::CreateDescriptorSet()
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

MeshRenderer::MeshRenderer(std::shared_ptr<vkg::Device> pDevice, std::string modelPath, std::string texturePath)
{
    m_pDevice = pDevice;
    m_mesh.m_sourceFile = modelPath;
    m_material.m_texturePath = texturePath;
}

std::vector<vk::DescriptorSetLayoutBinding> MeshRenderer::GetDescriptorSetLayoutBindings()
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

void MeshRenderer::UpdateUniformBuffers(vkg::UniformBufferObject& ubo)
{
    m_uniformBuffer.Map(0, sizeof(ubo));
    m_uniformBuffer.Copy(&ubo, sizeof(ubo));
    m_uniformBuffer.Unmap();
}

vk::DescriptorSet& MeshRenderer::GetDescriptorSet() { return m_vkDescriptorSet.get(); }

// -------------------------------------------------
// Components Methods
// -------------------------------------------------

void MeshRenderer::Initialize()
{
    CreateDataBuffers();
    CreateUniformBuffer();
    CreateMaterialTexture();
    CreateDescriptorSet();
}

void MeshRenderer::Shutdown()
{
    m_vkDescriptorSet.reset();

    // TODO: Make sure reassignement is good enough.
    m_materialTexture = vkg::resources::Image();
    m_materialTexture.SetDebugName("Material Texture");

    m_materialBuffer = vkg::resources::Buffer();

    m_uniformBuffer = vkg::resources::Buffer();
    m_vertexBuffer = vkg::resources::Buffer();
    m_indexBuffer = vkg::resources::Buffer();

    m_pDevice.reset();
}

bool MeshRenderer::Load()
{
    // Short circuit
    if (!m_mesh.Load())
    {
        std::cout << "Failed to load mesh ressource" << std::endl;
        return false;
    }

    if (!m_material.Load())
    {
        std::cout << "Failed to load material ressource" << std::endl;
        return false;
    }

    return true;
    // return m_mesh.Load() && m_material.Load();
}

void MeshRenderer::Unload()
{
    m_mesh.Unload();
    m_material.Unload();
}
} // namespace aln