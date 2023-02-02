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
    friend class SceneRenderer;

    ALN_REGISTER_TYPE();

  private:
    AssetHandle<SkeletalMesh> m_pMesh;
    AssetHandle<Skeleton> m_pSkeleton; // Animation Skeleton

    std::vector<Transform> m_boneTransforms; // Bone transforms, in global character space

    std::vector<glm::mat4x4> m_skinningTransforms;

    // AssetHandle<Skeleton> m_pSkeleton; // Rendering skeleton

    // TODO: Runtime state:
    // Bone mapping between animation and render skeletons
    std::vector<BoneIndex> m_animToRenderBonesMap;

    // Editor toggles
    /// @todo : Disable in release / move out to anoter class
    bool m_drawDebugSkeleton = true;
    bool m_drawRootBone = false;

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

    size_t GetBonesCount() const { return m_pSkeleton->GetBonesCount(); }

  private:
    void UpdateSkinningTransforms();

    void Initialize() override
    {
        assert(m_pMesh.IsLoaded() && m_pSkeleton.IsLoaded());
        assert(m_pMesh->m_bindPose.size() > 0);

        // TODO:
        m_skinningTransforms.resize(m_pMesh->m_bindPose.size());

        // Initialize the bone mapping
        /// @todo: For now skeletal meshes don't have a specific skeleton
        const auto boneCount = m_pSkeleton->GetBonesCount();
        m_animToRenderBonesMap.resize(boneCount, InvalidIndex);
        for (BoneIndex boneIdx = 0; boneIdx < boneCount; ++boneIdx)
        {
            // TODO: Actually map
            auto animBoneIdx = boneIdx;
            m_animToRenderBonesMap[boneIdx] = animBoneIdx;
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

    void Load(const LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Load(m_pMesh);
        loadingContext.m_pAssetService->Load(m_pSkeleton);
    }

    void Unload(const LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Unload(m_pMesh);
        loadingContext.m_pAssetService->Unload(m_pSkeleton);
    }

    bool UpdateLoadingStatus() override
    {
        if (m_pMesh.IsLoaded() && m_pSkeleton.IsLoaded())
        {
            m_status = Status::Loaded;
        }

        return IsLoaded();
    }

    /// @brief Draw this skeletal mesh's skeleton
    void DrawPose(DrawingContext& drawingContext) const
    {
        const Transform& worldTransform = GetWorldTransform();
        Transform boneWorldTransform = worldTransform * m_boneTransforms[0];

        /// @todo: Draw axis ?

        auto boneCount = m_boneTransforms.size();
        for (BoneIndex boneIndex = 1; boneIndex < boneCount; boneIndex++)
        {
            const auto parentBoneIndex = m_pSkeleton->GetParentBoneIndex(boneIndex);
            if (m_drawRootBone || parentBoneIndex != 0)
            {
                boneWorldTransform = worldTransform * m_boneTransforms[boneIndex];
                const Transform parentWorldTransform = worldTransform * m_boneTransforms[parentBoneIndex];
                drawingContext.DrawLine(parentWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), RGBColor::Red);
            }
        }
    }

    /// @brief Draw the bind pose
    void DrawBindPose(DrawingContext& drawingContext) const
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

            const auto parentBoneIndex = m_pSkeleton->GetParentBoneIndex(boneIndex);
            const Transform parentWorldTransform = worldTransform * bindPose[parentBoneIndex];

            drawingContext.DrawLine(parentWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), RGBColor::Red);
            /// @todo: Draw axis ?
        }
    }
};
} // namespace aln