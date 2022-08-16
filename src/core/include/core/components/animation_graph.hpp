#pragma once

#include <anim/animation_clip.hpp>
#include <anim/graph/graph.hpp>
#include <anim/pose.hpp>
#include <anim/skeleton.hpp>

#include <assets/handle.hpp>
#include <entities/component.hpp>

#include <assets/asset_service.hpp>
#include <assets/type_descriptors/handles.hpp>

#include <memory>

namespace aln
{

/// @brief Temporary placeholder for an animation graph instance. Simulate running a simple graph that only samples a single animation
struct PlaceHolderAnimationGraphInstance
{
    AssetHandle<AnimationClip> m_pAnimationClip; // Placeholder. TODO: Remove
    Pose m_pose;

    PlaceHolderAnimationGraphInstance(const Skeleton* pSkeleton) : m_pose(pSkeleton, Pose::InitialState::None) {}

    const Pose* GetPose(float time)
    {
        m_pAnimationClip->GetPose(time, &m_pose);
        return &m_pose;
    }
};

class AnimationGraphComponent : public entities::IComponent
{
    ALN_REGISTER_TYPE();

  private:
    AssetHandle<Skeleton> m_pSkeleton;                   // Animation skeleton
    PlaceHolderAnimationGraphInstance* m_pGraphInstance; // TODO: AnimationGraphInstance

  public:
    // TODO
    ~AnimationGraphComponent()
    {
        delete m_pGraphInstance;
    }

    // TODO: Should this be public ?
    void Construct(const entities::ComponentCreationContext& ctx) override
    {
        m_pSkeleton = AssetHandle<Skeleton>("tmp.anim");

        // TODO:
        m_pGraphInstance = new PlaceHolderAnimationGraphInstance(m_pSkeleton.get());
        m_pGraphInstance->m_pAnimationClip = AssetHandle<AnimationClip>("D:/Dev/allen_engine/assets/models/assets_export/Mike/Hello.anim");
    }

    void Load(const entities::LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Load(m_pGraphInstance->m_pAnimationClip);
        loadingContext.m_pAssetService->Load(m_pSkeleton);
    }

    void Unload(const entities::LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Unload(m_pGraphInstance->m_pAnimationClip);
        loadingContext.m_pAssetService->Unload(m_pSkeleton);
    }
};
} // namespace aln