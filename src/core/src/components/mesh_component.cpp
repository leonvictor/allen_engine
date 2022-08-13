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
    CreateUniformBuffer();
    m_pAssetManager->Initialize<Material>(m_pMaterial);
    CreateDescriptorSet();
}

void MeshComponent::Shutdown()
{
    m_vkDescriptorSet.reset();

    m_pAssetManager->Shutdown<Material>(m_pMaterial);

    m_uniformBuffer = vkg::resources::Buffer();

    m_pDevice.reset();
}

bool MeshComponent::Load()
{
    if (!m_pAssetManager->Load<Material>(m_pMaterial))
    {
        return false;
    }

    return true;
}

void MeshComponent::Unload()
{
    m_pAssetManager->Unload<Material>(m_pMaterial);
}
} // namespace aln
