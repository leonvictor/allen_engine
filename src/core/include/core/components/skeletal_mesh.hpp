#pragma once

#include "../skeletal_mesh.hpp"
#include "mesh.hpp"

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
    ALN_REGISTER_TYPE();

    //   private:
    //     AssetHandle<Skeleton> m_pSkeleton; // Rendering skeleton

    //     // TODO: Skeletal Mesh + Materials
    //     AssetHandle<SkeletalMesh> m_pSkeletalMesh; // TODO

    //     // TODO: Runtime state:
    //     // Bone mapping table between anim skeleton and mesh skeleton
    //     std::map<BoneIndex, BoneIndex> m_boneMapTable;

    //     // TODO: Skeletal Mesh Bone Matrices
    //     std::vector<glm::mat4> m_boneMatrices;

    //     // TODO: Procedural Bones Solver

    //   public:
    //     // TODO: Should this be public ?
    //     void Construct(const entities::ComponentCreationContext& ctx) override
    //     {
    //         m_pAssetManager = ctx.pAssetManager;
    //         m_pSkeleton = ctx.pAssetManager->Get<Skeleton>(""); // TODO

    //         // TODO:
    //     }

    //     void Initialize() override
    //     {
    //         m_pAssetManager->Initialize<Skeleton>(m_pSkeleton);

    //         // TODO:
    //     }

    //     void Shutdown() override
    //     {
    //         // TODO:
    //         m_pAssetManager->Shutdown<Skeleton>(m_pSkeleton);
    //     }

    //     bool Load() override
    //     {
    //         // TODO:
    //         m_pAssetManager->Load<Skeleton>(m_pSkeleton);

    //         return true;
    //     }

    //     void Unload() override
    //     {
    //         // TODO:
    //         m_pAssetManager->Unload<Skeleton>(m_pSkeleton);
    //     }
};
} // namespace aln