#pragma once

#include <anim/animation_clip.hpp>
#include <anim/pose.hpp>
#include <anim/skeleton.hpp>

#include <anim/graph/animation_graph_dataset.hpp>
#include <anim/graph/graph_definition.hpp>
#include <anim/graph/runtime_graph_instance.hpp>

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

  public:
    // ---------

    void Evaluate()
    {
        // TODO
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
        m_pGraphInstance = aln::New<RuntimeAnimationGraphInstance>(m_pGraphDefinition.get(), m_pGraphDataset.get());
        // TODO ...
    }

    void Shutdown() override
    {
        aln::Delete(m_pGraphInstance);
        // TODO ...
    }
};
} // namespace aln