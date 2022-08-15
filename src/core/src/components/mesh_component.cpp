#include "components/mesh_component.hpp"

#include <graphics/device.hpp>

namespace aln
{

void MeshComponent::CreateUniformBuffer()
{
    m_uniformBuffer = vkg::resources::Buffer(m_pDevice, sizeof(vkg::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void MeshComponent::UpdateUniformBuffers(vkg::UniformBufferObject& ubo)
{
    // TODO: We should only map once and unmap right before deletion
    m_uniformBuffer.Map(0, sizeof(ubo));
    m_uniformBuffer.Copy(&ubo, sizeof(ubo));
    m_uniformBuffer.Unmap();
}

vk::DescriptorSet& MeshComponent::GetDescriptorSet() { return m_vkDescriptorSet.get(); }

// -------------------------------------------------
// Components Methods
// -------------------------------------------------

void MeshComponent::Initialize()
{
    assert(m_pMaterial.IsLoaded());
    CreateUniformBuffer();
    CreateDescriptorSet();
}

void MeshComponent::Shutdown()
{
    m_vkDescriptorSet.reset();
    m_uniformBuffer = vkg::resources::Buffer();

    m_pDevice.reset();
}

void MeshComponent::Load()
{
    m_pAssetManager->Load(m_pMaterial);
}

void MeshComponent::Unload()
{
    m_pAssetManager->Unload(m_pMaterial);
}
} // namespace aln
