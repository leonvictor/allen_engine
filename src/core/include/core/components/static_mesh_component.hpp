#pragma once

#include "../static_mesh.hpp"
#include "mesh_component.hpp"

namespace aln
{

/// @brief The StaticMeshComponent component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class StaticMeshComponent : public MeshComponent
{
    ALN_REGISTER_TYPE();

  private:
    AssetHandle<StaticMesh> m_pMesh;

  public:
    inline const StaticMesh* GetMesh() const { return m_pMesh.get(); }

    virtual void Construct(const entities::ComponentCreationContext& ctx) override
    {
        MeshComponent::Construct(ctx);
        m_pMesh = ctx.pAssetManager->Get<StaticMesh>("D:/Dev/allen_engine/assets/models/assets_export/cube/cube.mesh");
        m_pMaterial->SetAlbedoMap(m_pAssetManager->Get<Texture>("D:/Dev/allen_engine/assets/models/assets_export/cube/chalet.text"));
    }

    // TODO: Descriptor allocation and update is managed by the swapchain.
    // We could extract this part and use a method where each objects requests a descriptor from the pool ?
    // Each descriptable Component should register its descriptor to its parent object
    // *before* creation
    void CreateDescriptorSet() override
    {
        m_vkDescriptorSet = m_pDevice->AllocateDescriptorSet<StaticMeshComponent>();
        m_pDevice->SetDebugUtilsObjectName(m_vkDescriptorSet.get(), "StaticMeshComponent Descriptor Set");

        std::array<vk::WriteDescriptorSet, 3> writeDescriptors = {};

        writeDescriptors[0].dstSet = m_vkDescriptorSet.get();
        writeDescriptors[0].dstBinding = 0;
        writeDescriptors[0].dstArrayElement = 0;
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = m_uniformBuffer.GetDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        writeDescriptors[1].dstSet = m_vkDescriptorSet.get();
        writeDescriptors[1].dstBinding = 1;
        writeDescriptors[1].dstArrayElement = 0;
        writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescriptors[1].descriptorCount = 1;
        auto textureDescriptor = m_pMaterial->GetAlbedoMap()->GetDescriptor();
        writeDescriptors[1].pImageInfo = &textureDescriptor;

        // TODO: Materials presumably don't change so they don't need a binding
        writeDescriptors[2].dstSet = m_vkDescriptorSet.get();
        writeDescriptors[2].dstBinding = 2;
        writeDescriptors[2].dstArrayElement = 0;
        writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[2].descriptorCount = 1;
        auto materialDescriptor = m_pMaterial->GetBuffer().GetDescriptor();
        writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?

        m_pDevice->GetVkDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }

    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{
            {
                // We need to access the ubo in the fragment shader aswell now (because it contains light direction)
                // TODO: There is probably a cleaner way (a descriptor for all light sources for example ?)

                // UBO
                .binding = 0,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                .pImmutableSamplers = nullptr,
            },
            {
                // Sampler
                .binding = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
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

  private:
    void Initialize() override
    {
        m_pAssetManager->Initialize<StaticMesh>(m_pMesh);
        MeshComponent::Initialize();
    }

    void Shutdown() override
    {
        m_pAssetManager->Shutdown<StaticMesh>(m_pMesh);
        MeshComponent::Shutdown();
    }

    bool Load() override
    {
        if (!m_pAssetManager->Load<StaticMesh>(m_pMesh))
        {
            std::cout << "Failed to load mesh ressource" << std::endl;
            return false;
        }
        return MeshComponent::Load();
    }

    void Unload() override
    {
        m_pAssetManager->Unload<StaticMesh>(m_pMesh);
        MeshComponent::Unload();
    }
};
} // namespace aln