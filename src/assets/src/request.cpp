#include "request.hpp"
#include "status.hpp"

namespace aln
{
class AssetService;

void AssetRequest::Load()
{
    // Load the resource
    if (!m_pLoader->LoadAsset(m_pAssetRecord))
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
    assert(m_status == State::Installing);
    assert(m_pLoader != nullptr);

    m_pLoader->InstallAsset(m_pAssetRecord->GetAssetID(), m_pAssetRecord, m_dependencies);
    m_dependencies.clear();

    m_pAssetRecord->m_status = AssetStatus::Loaded;
    m_status = State::Complete;
}

void AssetRequest::Unload()
{
    m_pLoader->UnloadAsset(m_pAssetRecord);

    for (auto& dependencyID : m_pAssetRecord->GetDependencies())
    {
        auto& dependencyHandle = m_dependencies.emplace_back(IAssetHandle(dependencyID));
        m_requestAssetUnload(dependencyHandle);
    }
    m_pAssetRecord->m_status = AssetStatus::Unloaded;
    m_status = State::Complete;
}
} // namespace aln