#pragma once

#include "../skeletal_mesh.hpp"
#include "mesh_component.hpp"

#include <anim/skeleton.hpp>
#include <assets/asset_service.hpp>
#include <common/drawing_context.hpp>
#include <common/maths/matrix4x4.hpp>
#include <common/transform.hpp>
#include <entities/spatial_component.hpp>

namespace aln
{

class SkeletalMeshComponent : public MeshComponent
{
    friend class WorldRenderingSystem;
    friend class WorldRenderer;

    ALN_REGISTER_TYPE();

  private:
    AssetHandle<SkeletalMesh> m_pMesh;
    AssetHandle<Skeleton> m_pSkeleton; // Animation Skeleton

    // Rendering bone transforms in global character space
    Vector<Transform> m_boneTransforms;
    Vector<Matrix4x4> m_skinningTransforms;

    // Bone mapping between animation and render skeletons
    Vector<BoneIndex> m_animToRenderBonesMap;

    // Editor toggles
    /// @todo : Disable in release / move out to anoter class
    bool m_drawDebugSkeleton = false;
    bool m_drawRootBone = false;

    // TODO: Procedural Bones Solver

  public:
    inline const SkeletalMesh* GetMesh() const { return m_pMesh.get(); }
    inline const Skeleton* GetSkeleton() const { return m_pSkeleton.get(); }
    inline bool HasSkeletonSet() const { return m_pSkeleton.IsValid(); }

    void SetMesh(const AssetID& meshID)
    {
        assert(IsUnloaded());
        m_pMesh = AssetHandle<SkeletalMesh>(meshID);
    }

    void SetSkeleton(const AssetID& skeletonID)
    {
        assert(IsUnloaded());
        m_pSkeleton = AssetHandle<Skeleton>(skeletonID);
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

    /// @brief Reset the pose to the rendering bind pose
    /// @todo We could alternatively reset to the animation skeleton's reference pose
    void ResetPose()
    {
        m_boneTransforms = m_pMesh->GetBindPose();
    }

    /// @brief Number of render bones
    size_t GetBonesCount() const { return m_boneTransforms.size(); }

  private:
    /// @brief Compute the skinning matrices of the rendering skeleton from the current associated transforms
    void UpdateSkinningTransforms();

    void Initialize() override
    {
        assert(m_pMesh.IsLoaded() && m_pSkeleton.IsLoaded());
        assert(m_pMesh->m_bindPose.size() > 0);

        // Initialize bone mapping
        const auto animBoneCount = m_pSkeleton->GetBonesCount();
        m_animToRenderBonesMap.resize(animBoneCount, InvalidIndex);
        for (BoneIndex boneIdx = 0; boneIdx < animBoneCount; ++boneIdx)
        {
            const auto& animBoneName = m_pSkeleton->GetBoneName(boneIdx);
            const auto& boneIndex = m_pMesh->GetBoneIndex(animBoneName);
            m_animToRenderBonesMap[boneIdx] = boneIndex;
        }

        m_boneTransforms = m_pMesh->GetBindPose();

        // TODO: Maybe set to anim's skeleton reference pose instead ?
        /*m_boneTransforms.resize(m_pMesh->GetBonesCount());
        auto pose = Pose(m_pSkeleton.get());
        pose.CalculateGlobalTransforms();
        SetPose(&pose);*/

        m_skinningTransforms.resize(m_pMesh->m_bindPose.size());
        UpdateSkinningTransforms();

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
        if (m_pMesh.IsValid())
        {
            loadingContext.m_pAssetService->Load(m_pMesh);
        }
        if (m_pSkeleton.IsValid())
        {
            loadingContext.m_pAssetService->Load(m_pSkeleton);
        }
    }

    void Unload(const LoadingContext& loadingContext) override
    {
        if (m_pMesh.IsValid())
        {
            loadingContext.m_pAssetService->Unload(m_pMesh);
        }
        if (m_pSkeleton.IsValid())
        {
            loadingContext.m_pAssetService->Unload(m_pSkeleton);
        }
    }

    bool UpdateLoadingStatus() override
    {
        if (m_pMesh.IsLoaded() && m_pSkeleton.IsLoaded())
        {
            m_status = Status::Loaded;
        }
        else if (m_pMesh.HasFailedLoading() || m_pSkeleton.HasFailedLoading())
        {
            m_status = Status::LoadingFailed;
        }
        return IsLoaded();
    }

    /// @brief Draw this skeletal mesh's render skeleton
    void DrawPose(DrawingContext& drawingContext) const
    {
        const Transform& worldTransform = GetWorldTransform();
        Transform boneWorldTransform = worldTransform * m_boneTransforms[0];

        /// @todo: Draw axis ?

        auto boneCount = m_boneTransforms.size();
        for (BoneIndex boneIndex = 1; boneIndex < boneCount; boneIndex++)
        {
            const auto parentBoneIndex = m_pMesh->GetParentBoneIndex(boneIndex);
            boneWorldTransform = worldTransform * m_boneTransforms[boneIndex];
            const Transform parentWorldTransform = worldTransform * m_boneTransforms[parentBoneIndex];
            drawingContext.DrawLine(parentWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), RGBColor::Red);
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

            const auto parentBoneIndex = m_pMesh->GetParentBoneIndex(boneIndex);
            const Transform parentWorldTransform = worldTransform * bindPose[parentBoneIndex];

            drawingContext.DrawLine(parentWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), RGBColor::Red);
            /// @todo: Draw axis ?
        }
    }
};
} // namespace aln