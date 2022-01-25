#pragma once

#include <anim/animation_clip.hpp>
#include <anim/skeleton.hpp>

#include <entities/spatial_component.hpp>

#include <assets/handle.hpp>
#include <assets/type_descriptors/handles.hpp>

#include <memory>

namespace aln
{
class SkeletalMeshComponent : public entities::SpatialComponent
{
    ALN_REGISTER_TYPE();

  private:
    std::shared_ptr<AssetManager> m_pAssetManager = nullptr;
    AssetHandle<Skeleton> m_pSkeleton; // Animation skeleton
                                       // TODO: Skeletal Mesh + Materials
                                       // TODO: Skeletal Mesh Bone Matrices
                                       // TODO: Procedural Bones Solver
                                       // TODO: AnimationGraphInstance

  public:
    // TODO: Should this be public ?
    void Construct(const entities::ComponentCreationContext& ctx) override
    {
        m_pAssetManager = ctx.pAssetManager;
        // TODO:
    }

    void Initialize() override
    {
        // TODO:
    }

    void Shutdown() override
    {
        // TODO:
    }

    bool Load() override
    {
        // TODO:
        return true;
    }

    void Unload() override
    {
        // TODO:
    }
};
} // namespace aln