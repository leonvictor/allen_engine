#pragma once

#include <anim/animation_clip.hpp>
#include <anim/graph/graph.hpp>
#include <anim/pose.hpp>
#include <anim/skeleton.hpp>

#include <assets/handle.hpp>
#include <entities/component.hpp>

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
    std::shared_ptr<AssetManager> m_pAssetManager = nullptr;
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
        m_pAssetManager = ctx.pAssetManager;
        m_pSkeleton = ctx.pAssetManager->Get<Skeleton>("tmp.anim"); // TODO

        // TODO:
        m_pGraphInstance = new PlaceHolderAnimationGraphInstance(m_pSkeleton.get());
        m_pGraphInstance->m_pAnimationClip = ctx.pAssetManager->Get<AnimationClip>("D:/Dev/allen_engine/assets/models/assets_export/Mike/Hello.anim");
    }

    void Initialize() override
    {
        m_pAssetManager->Initialize<AnimationClip>(m_pGraphInstance->m_pAnimationClip);
        m_pAssetManager->Initialize<Skeleton>(m_pSkeleton);
    }

    void Shutdown() override
    {
        m_pAssetManager->Shutdown<AnimationClip>(m_pGraphInstance->m_pAnimationClip);
        m_pAssetManager->Shutdown<Skeleton>(m_pSkeleton);
    }

    bool Load() override
    {
        m_pAssetManager->Load<AnimationClip>(m_pGraphInstance->m_pAnimationClip);
        m_pAssetManager->Load<Skeleton>(m_pSkeleton);
        return true;
    }

    void Unload() override
    {
        m_pAssetManager->Unload<AnimationClip>(m_pGraphInstance->m_pAnimationClip);
        m_pAssetManager->Unload<Skeleton>(m_pSkeleton);
    }
};
} // namespace aln