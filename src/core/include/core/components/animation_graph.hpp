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
    Transform m_rootMotionDelta = Transform::Identity;

    Percentage m_previousAnimTime = 0.0f;
    Percentage m_animTime = 0.0f;

  public:
    inline const Pose* GetPose() { return m_pPose; }
    inline const Transform& GetRootMotionDelta() { return m_rootMotionDelta; }

    // --------- Control Parameters

    NodeIndex GetControlParameterIndex(const StringID& parameterName) const
    {
        return m_pGraphInstance->GetControlParameterIndex(parameterName);
    }

    template<typename T>
    void SetControlParameterValue(NodeIndex parameterIndex, const T& value)
    {
        m_pGraphInstance->SetControlParameterValue(m_graphContext, parameterIndex, value);
    }

    template<typename T>
    const T& GetControlParameterValue(NodeIndex parameterIndex) const
    {
        return m_pGraphInstance->GetControlParameterValue<T>(m_graphContext, parameterIndex);
    }

    // ---- Events
    const SampledEventsBuffer& GetSampledEventsBuffer() const { return m_graphContext.m_sampledEventsBuffer; } 

    // --------- Evaluation/Execution
    /// @todo Maybe this could only be accessed by the animation system ?
    
    /// @brief Run through the animation graph recording tasks
    /// @param deltaTime
    void Evaluate(float deltaTime, const Transform& characterWorldTransform)
    {
        ZoneScoped;

        assert(m_graphContext.IsValid());

        // TODO
        m_graphContext.Update(deltaTime, characterWorldTransform);

        m_pTaskSystem->Reset();
     
        const auto result = m_pGraphInstance->Update(m_graphContext);
        m_rootMotionDelta = result.m_rootMotionDelta;
    }

    void ExecuteTasks()
    {
        ZoneScoped;
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