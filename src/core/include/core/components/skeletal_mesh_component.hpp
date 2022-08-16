#pragma once

#include "../drawing_context.hpp"
#include "../skeletal_mesh.hpp"
#include "mesh_component.hpp"

#include <common/transform.hpp>

#include <anim/animation_clip.hpp>
#include <anim/skeleton.hpp>

#include <entities/spatial_component.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <memory>

namespace aln
{
class SkeletalMeshComponent : public MeshComponent
{
    friend class GraphicsSystem;

    ALN_REGISTER_TYPE();

  private:
    AssetHandle<SkeletalMesh> m_pMesh;
    AssetHandle<Skeleton> m_pSkeleton; // Animation Skeleton

    std::vector<Transform> m_boneTransforms; // Bone transforms, in global character space

    std::vector<glm::mat4x4> m_skinningTransforms;
    vkg::resources::Buffer m_skinningBuffer;
    vk::UniqueDescriptorSet m_vkSkinDescriptorSet;

    // AssetHandle<Skeleton> m_pSkeleton; // Rendering skeleton

    // // TODO: Runtime state:
    // Bone mapping between animation and render skeletons
    std::vector<BoneIndex> m_animToRenderBonesMap;

    // TODO: Procedural Bones Solver

  public:
    inline const SkeletalMesh* GetMesh() const { return m_pMesh.get(); }
    inline const Skeleton* GetSkeleton() const { return m_pSkeleton.get(); }

    void SetMesh(const std::string& path) override
    {
        assert(IsUnloaded());
        m_pMesh = AssetHandle<SkeletalMesh>(path);
    }

    void SetSkeleton(const std::string& path)
    {
        assert(IsUnloaded());
        m_pSkeleton = AssetHandle<Skeleton>(path);
    }

    void SetPose(const Pose* pPose)
    {
        assert(pPose != nullptr);
        // Map from animation to render bones
        auto animBoneCount = pPose->GetBonesCount();
        for (BoneIndex animBoneIdx = 0; animBoneIdx < animBoneCount; ++animBoneIdx)
        {
            auto renderBoneIdx = m_animToRenderBonesMap[animBoneIdx];
            m_boneTransforms[renderBoneIdx] = pPose->GetGlobalTransform(animBoneIdx);
        }
    }

    /// @brief Reset the pose to the reference one
    void ResetPose()
    {
        m_boneTransforms = m_pMesh->GetBindPose();
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
        auto skinningBufferDescriptor = m_skinningBuffer.GetDescriptor();
        writeDescriptors[3].pBufferInfo = &skinningBufferDescriptor; // TODO: Replace w/ push constants ?

        m_pDevice->GetVkDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }

  private:
    void UpdateSkinningBuffer();

    void Initialize() override
    {
        assert(m_pMesh.IsLoaded() && m_pSkeleton.IsLoaded());
        assert(m_pMesh->m_bindPose.size() > 0);

        // TODO:
        m_skinningTransforms.resize(m_pMesh->m_bindPose.size());
        // TODO: The buffer should be device-local
        m_skinningBuffer = vkg::resources::Buffer(m_pDevice, m_pMesh->m_inverseBindPose.size() * sizeof(glm::mat4x4), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        // Initialize the bone mapping
        const auto boneCount = m_pSkeleton->GetBonesCount();
        m_animToRenderBonesMap.resize(boneCount, InvalidIndex);
        for (auto boneIdx = 0; boneIdx < boneCount; ++boneIdx)
        {
            auto animBoneIdx = m_pSkeleton->GetBone(boneIdx)->GetIndex();
            m_animToRenderBonesMap[boneIdx] = animBoneIdx; // TODO !!
        }

        // TODO: Set to bind pose
        m_boneTransforms = m_pSkeleton->GetGlobalReferencePose();
        MeshComponent::Initialize();
    }

    void Shutdown() override
    {
        m_animToRenderBonesMap.clear();
        m_skinningTransforms.clear();
        m_boneTransforms.clear();
        MeshComponent::Shutdown();
    }

    void Load(const entities::LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Load(m_pMesh);
        loadingContext.m_pAssetService->Load(m_pSkeleton);
        MeshComponent::Load(loadingContext);
    }

    void Unload(const entities::LoadingContext& loadingContext) override
    {
        m_skinningTransforms.clear();
        loadingContext.m_pAssetService->Unload(m_pMesh);
        MeshComponent::Unload(loadingContext);
    }

    bool UpdateLoadingStatus() override
    {
        if (m_pMesh.IsLoaded() && m_pSkeleton.IsLoaded() && m_pMaterial.IsLoaded())
        {
            m_status = Status::Loaded;
        }

        return IsLoaded();
    }

    /// @brief Draw this skeletal mesh's skeleton
    void DrawPose(DrawingContext& drawingContext)
    {
        const Transform& worldTransform = GetWorldTransform();
        Transform boneWorldTransform = worldTransform * m_boneTransforms[0];

        /// @todo: Draw axis ?

        auto boneCount = m_boneTransforms.size();
        for (BoneIndex boneIndex = 1; boneIndex < boneCount; boneIndex++)
        {
            boneWorldTransform = worldTransform * m_boneTransforms[boneIndex];

            const auto parentBoneIndex = m_pSkeleton->GetBone(boneIndex)->GetParentIndex();
            const Transform parentWorldTransform = worldTransform * m_boneTransforms[parentBoneIndex];

            drawingContext.DrawLine(parentWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), RGBColor::Red);
        }
    }

    /// @brief Draw the bind pose
    void DrawBindPose(DrawingContext& drawingContext)
    {
        // Debug: Draw a single line from 0 to somewhere
        drawingContext.DrawCoordinateAxis(Transform::Identity);

        const auto& bindPose = m_pMesh->GetBindPose();
        const Transform& worldTransform = GetWorldTransform();
        Transform boneWorldTransform = worldTransform * bindPose[0];
        /// @todo: Draw root

        auto boneCount = bindPose.size();
        for (BoneIndex boneIndex = 1; boneIndex < boneCount; boneIndex++)
        {
            boneWorldTransform = worldTransform * bindPose[boneIndex];

            const auto parentBoneIndex = m_pSkeleton->GetBone(boneIndex)->GetParentIndex();
            const Transform parentWorldTransform = worldTransform * bindPose[parentBoneIndex];

            drawingContext.DrawLine(parentWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), RGBColor::Red);
            /// @todo: Draw axis ?
        }
    }
};
} // namespace aln