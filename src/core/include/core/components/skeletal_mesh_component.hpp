#pragma once

#include "../skeletal_mesh.hpp"
#include "mesh_component.hpp"

#include <anim/animation_clip.hpp>
#include <anim/skeleton.hpp>

#include <entities/spatial_component.hpp>

#include <assets/handle.hpp>
#include <assets/type_descriptors/handles.hpp>

#include <glm/mat4x4.hpp>

#include <memory>
#include <vector>

namespace aln
{
class SkeletalMeshComponent : public MeshComponent
{
    friend class GraphicsSystem;

    ALN_REGISTER_TYPE();

  private:
    vk::UniqueDescriptorSet m_vkSkinDescriptorSet;
    AssetHandle<SkeletalMesh> m_pMesh;

    std::vector<glm::mat4x4> m_skinningTransforms;

    // AssetHandle<Skeleton> m_pSkeleton; // Rendering skeleton

    // // TODO: Runtime state:
    // // Bone mapping table between anim skeleton and mesh skeleton
    // std::map<BoneIndex, BoneIndex> m_boneMapTable;

    // // TODO: Skeletal Mesh Bone Matrices
    // std::vector<glm::mat4> m_boneMatrices;

    // TODO: Procedural Bones Solver

  public:
    inline const SkeletalMesh* GetMesh() const { return m_pMesh.get(); }

    void Construct(const entities::ComponentCreationContext& ctx) override
    {
        MeshComponent::Construct(ctx);
        m_pMesh = ctx.pAssetManager->Get<SkeletalMesh>(ctx.defaultModelPath);
        m_pMaterial = ctx.pAssetManager->Get<Material>("DefaultMaterial");
        m_pMaterial->SetAlbedoMap(m_pAssetManager->Get<Texture>(ctx.defaultTexturePath));
        m_pDevice = ctx.graphicsDevice;
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
            },
            {
                .binding = 3,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex,
            }};

        return bindings;
    }

    void CreateDescriptorSet() override
    {
        m_vkDescriptorSet = m_pDevice->AllocateDescriptorSet<SkeletalMeshComponent>();
        m_pDevice->SetDebugUtilsObjectName(m_vkDescriptorSet.get(), "SkeletalMeshComponent Descriptor Set");

        std::array<vk::WriteDescriptorSet, 4> writeDescriptors = {};

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

        writeDescriptors[3].dstSet = m_vkDescriptorSet.get();
        writeDescriptors[3].dstBinding = 3;
        writeDescriptors[3].dstArrayElement = 0;
        writeDescriptors[3].descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescriptors[3].descriptorCount = 1;
        auto skinningBufferDescriptor = m_pMesh->GetSkinningBuffer().GetDescriptor();
        writeDescriptors[3].pBufferInfo = &skinningBufferDescriptor; // TODO: Replace w/ push constants ?

        m_pDevice->GetVkDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }

  private:
    void UpdateSkinningBuffer()
    {
        // Update the transform matrices of skinning matrices
        for (auto i = 0; i < m_pMesh->m_bindPose.size(); ++i)
        {
            // TODO: For now we always use the bind pose...
            const Transform transform = m_pMesh->GetInverseBindPose()[i] * m_pMesh->GetBindPose()[i];
            m_skinningTransforms[i] = transform.ToMatrix();
        }

        // Upload the new data to gpu
        // TODO: the buffer is host visible for now, which is not optimal
        m_pMesh->m_skinningBuffer.Map(0, sizeof(glm::mat4x4) * m_skinningTransforms.size());
        m_pMesh->m_skinningBuffer.Copy(m_skinningTransforms);
        m_pMesh->m_skinningBuffer.Unmap();
    }

    void Initialize() override
    {
        // TODO:
        // m_pAssetManager->Initialize<Skeleton>(m_pSkeleton);
        m_pAssetManager->Initialize<SkeletalMesh>(m_pMesh);
        MeshComponent::Initialize();
    }

    void Shutdown() override
    {
        // TODO:
        // m_pAssetManager->Shutdown<Skeleton>(m_pSkeleton);
        m_pAssetManager->Shutdown<SkeletalMesh>(m_pMesh);
        MeshComponent::Shutdown();
    }

    bool Load() override
    {
        // TODO:
        // m_pAssetManager->Load<Skeleton>(m_pSkeleton);

        if (!m_pAssetManager->Load<SkeletalMesh>(m_pMesh))
        {
            std::cout << "Failed to load mesh ressource" << std::endl;
            return false;
        }

        m_skinningTransforms.resize(m_pMesh->m_bindPose.size());

        return MeshComponent::Load();
    }

    void Unload() override
    {
        // TODO:
        // m_pAssetManager->Unload<Skeleton>(m_pSkeleton);
        m_skinningTransforms.clear();
        m_pAssetManager->Unload<SkeletalMesh>(m_pMesh);
        MeshComponent::Unload();
    }
};
} // namespace aln