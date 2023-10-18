#include "request.hpp"
#include "status.hpp"

namespace aln
{
class AssetService;

void AssetRequest::Load()
{
    ZoneScoped;

    IAssetLoader::RequestContext ctx = {
        .m_threadIdx = m_threadIdx,
        .m_pSourceRequest = this,
    };

    if (!m_pLoader->LoadAsset(ctx, m_pAssetRecord)) // Load the resource
    {
        // TODO: Loading failed. Handle it !
        m_status = State::Failed;
        assert(0);
    }

    if (!m_pAssetRecord->HasDependencies())
    {
        // Loading finished and no dependencies to wait for
        m_status = State::Installing;
    }
    else
    {
        m_status = State::WaitingForDependencies;

        // Start loading the dependencies
        auto& dependencies = m_pAssetRecord->GetDependencies();
        m_dependencies.reserve(dependencies.size());
        for (auto& dependencyID : dependencies)
        {
            auto& dependencyHandle = m_dependencies.emplace_back(IAssetHandle(dependencyID));
            m_requestAssetLoad(dependencyHandle);
        }
    }
}

void AssetRequest::WaitForDependencies()
{
    ZoneScoped;

    bool dependenciesLoaded = true;
    for (auto& dependencyHandle : m_dependencies)
    {
        if (dependencyHandle.IsUnloaded())
        {
            dependenciesLoaded = false;
            // TODO: Handle the case where some dependency failed loading
        }
    }

    if (dependenciesLoaded)
    {
        m_status = State::Installing;
    }
}

void AssetRequest::Install()
{
    ZoneScoped;

    assert(m_status == State::Installing);
    assert(m_pLoader != nullptr);

    if ((HasTouchedGPUTransferQueue() && !m_pRenderDevice->GetVkDevice().getSemaphoreCounterValue(m_pTransferQueueCommandsSemaphore.get()).value) ||
        (HasTouchedGPUGraphicsQueue() && !m_pRenderDevice->GetVkDevice().getSemaphoreCounterValue(m_pGraphicsQueueCommandsSemaphore.get()).value))
    {
        // We've already started loading. Wait on the semaphore being signaled to actually mark the asset as ready
        return;
    }

    m_pLoader->InstallAsset(m_pAssetRecord->GetAssetID(), m_pAssetRecord, m_dependencies);
    m_dependencies.clear();

    m_pAssetRecord->m_status = AssetStatus::Loaded;
    m_status = State::Complete;
}

void AssetRequest::Unload()
{
    ZoneScoped;

    m_pLoader->UnloadAsset(m_pAssetRecord);

    for (auto& dependencyID : m_pAssetRecord->GetDependencies())
    {
        auto& dependencyHandle = m_dependencies.emplace_back(IAssetHandle(dependencyID));
        m_requestAssetUnload(dependencyHandle);
    }

    // To be perfectly symetric unloading should go through an "uninstalling" step which is not necessary
    m_pAssetRecord->m_dependencies.clear();

    m_pAssetRecord->m_status = AssetStatus::Unloaded;
    m_status = State::Complete;
}
} // namespace aln