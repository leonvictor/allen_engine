#pragma once

#include <anim/animation_clip.hpp>
#include <anim/pose.hpp>
#include <anim/skeleton.hpp>

#include <anim/graph/animation_graph_dataset.hpp>
#include <anim/graph/graph_context.hpp>
#include <anim/graph/graph_definition.hpp>
#include <anim/graph/runtime_graph_instance.hpp>
#include <anim/graph/task_system.hpp>

#include <assets/asset_service.hpp>
#include <assets/handle.hpp>
#include <entities/component.hpp>

#include <memory>

namespace aln
{

class AnimationGraphComponent : public IComponent
{
    ALN_REGISTER_TYPE();

  private:
    AssetHandle<Skeleton> m_pSkeleton; // Animation skeleton
    AssetHandle<AnimationGraphDefinition> m_pGraphDefinition;
    AssetHandle<AnimationGraphDataset> m_pGraphDataset;

    RuntimeAnimationGraphInstance* m_pGraphInstance = nullptr;
    GraphContext m_graphContext;
    TaskSystem* m_pTaskSystem;

    Pose* m_pPose = nullptr;
    Percentage m_previousAnimTime = 0.0f;
    Percentage m_animTime = 0.0f;

  public:
    inline const Pose* GetPose() { return m_pPose; }

    // ---------

    /// @brief Run through the animation graph recording tasks
    /// @param deltaTime
    void Evaluate(float deltaTime, const Transform& characterWorldTransform)
    {
        assert(m_graphContext.IsValid());

        // TODO
        m_graphContext.Update(deltaTime, characterWorldTransform);

        m_pTaskSystem->Reset();
        m_pGraphInstance->Update(m_graphContext);
    }

    void ExecuteTasks()
    {
        m_pTaskSystem->ExecuteTasks(m_graphContext.m_deltaTime, m_graphContext.m_worldTransform, m_pPose);
    }

    // --------- Component methods
    void Load(const LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Load(m_pSkeleton);
        loadingContext.m_pAssetService->Load(m_pGraphDefinition);
        loadingContext.m_pAssetService->Load(m_pGraphDataset);
    }

    void Unload(const LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Unload(m_pGraphDataset);
        loadingContext.m_pAssetService->Unload(m_pGraphDefinition);
        loadingContext.m_pAssetService->Unload(m_pSkeleton);
    }

    bool UpdateLoadingStatus() override
    {
        if (m_pSkeleton.IsLoaded() && m_pGraphDataset.IsLoaded() && m_pGraphDefinition.IsLoaded())
        {
            m_status = Status::Loaded;
        }
        else if (!m_pSkeleton.IsValid() || m_pSkeleton.HasFailedLoading() ||
                 !m_pGraphDataset.IsValid() || m_pGraphDataset.HasFailedLoading() ||
                 !m_pGraphDefinition.IsValid() || m_pGraphDefinition.HasFailedLoading())
        {
            m_status = Status::LoadingFailed;
        }
        return IsLoaded();
    }

    void Initialize() override
    {
        m_pPose = aln::New<Pose>(m_pSkeleton.get());
        m_pTaskSystem = aln::New<TaskSystem>(m_pSkeleton.get());
        m_pGraphInstance = aln::New<RuntimeAnimationGraphInstance>(m_pGraphDefinition.get(), m_pGraphDataset.get());

        m_graphContext.Initialize(m_pTaskSystem, m_pPose);
        m_pGraphInstance->Initialize(m_graphContext);
    }

    void Shutdown() override
    {
        m_pGraphInstance->Shutdown();
        m_graphContext.Shutdown();

        aln::Delete(m_pGraphInstance);
        aln::Delete(m_pTaskSystem);
        aln::Delete(m_pPose);
    }
};
} // namespace aln