#include "components/static_mesh.hpp"

#include <graphics/device.hpp>

namespace aln
{

void StaticMeshComponent::CreateUniformBuffer()
{
    m_uniformBuffer = vkg::resources::Buffer(m_pDevice, sizeof(vkg::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

// TODO: Descriptor allocation and update is managed by the swapchain.
// We could extract this part and use a method where each objects requests a descriptor from the pool ?
// Each descriptable Component should register its descriptor to its parent object
// *before* creation
void StaticMeshComponent::CreateDescriptorSet()
{
    m_vkDescriptorSet = m_pDevice->AllocateDescriptorSet<StaticMeshComponent>();
    m_pDevice->SetDebugUtilsObjectName(m_vkDescriptorSet.get(), "StaticMeshComponent Descriptor Set");

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
    auto textureDescriptor = m_pMaterial->m_albedoMap->m_image.GetDescriptor();
    writeDescriptors[1].pImageInfo = &textureDescriptor;

    // TODO: Materials presumably don't change so they don't need a binding
    writeDescriptors[2].dstSet = m_vkDescriptorSet.get();
    writeDescriptors[2].dstBinding = 2;
    writeDescriptors[2].dstArrayElement = 0;
    writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptors[2].descriptorCount = 1;
    auto materialDescriptor = m_pMaterial->m_buffer.GetDescriptor();
    writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?

    m_pDevice->GetVkDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
}

std::vector<vk::DescriptorSetLayoutBinding> StaticMeshComponent::GetDescriptorSetLayoutBindings()
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

void StaticMeshComponent::UpdateUniformBuffers(vkg::UniformBufferObject& ubo)
{
    m_uniformBuffer.Map(0, sizeof(ubo));
    m_uniformBuffer.Copy(&ubo, sizeof(ubo));
    m_uniformBuffer.Unmap();
}

vk::DescriptorSet& StaticMeshComponent::GetDescriptorSet() { return m_vkDescriptorSet.get(); }

// -------------------------------------------------
// Components Methods
// -------------------------------------------------

void StaticMeshComponent::Initialize()
{
    m_pAssetManager->Initialize<Mesh>(m_pMesh);
    CreateUniformBuffer();
    m_pAssetManager->Initialize<Material>(m_pMaterial);
    CreateDescriptorSet();
}

void StaticMeshComponent::Shutdown()
{
    m_vkDescriptorSet.reset();

    m_pAssetManager->Shutdown<Material>(m_pMaterial);

    m_uniformBuffer = vkg::resources::Buffer();
    m_pAssetManager->Shutdown<Mesh>(m_pMesh);

    m_pDevice.reset();
}

bool StaticMeshComponent::Load()
{
    // Short circuit
    if (!m_pAssetManager->Load<Mesh>(m_pMesh))
    {
        std::cout << "Failed to load mesh ressource" << std::endl;
        return false;
    }

    if (!m_pAssetManager->Load<Material>(m_pMaterial))
    {
        return false;
    }

    return true;
}

void StaticMeshComponent::Unload()
{
    m_pAssetManager->Unload<Mesh>(m_pMesh);
    m_pAssetManager->Unload<Material>(m_pMaterial);
}
} // namespace aln

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::StaticMeshComponent)
ALN_REFLECT_MEMBER(m_pMesh)
ALN_REFLECT_MEMBER(m_pMaterial)
ALN_REGISTER_IMPL_END()