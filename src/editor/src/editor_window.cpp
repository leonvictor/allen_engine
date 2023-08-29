#include "editor_window.hpp"

#include <assets/asset_service.hpp>

#include <assert.h>

namespace aln
{
    void IEditorWindow::Initialize(EditorWindowContext* pEditorWindowContext)
    {
        m_pEditorWindowContext = pEditorWindowContext;
    }

    void IEditorWindow::Shutdown()
    {
        m_pEditorWindowContext = nullptr;
    }

    void IEditorWindow::LoadAsset(IAssetHandle& assetHandle)
    {
        assert(assetHandle.IsUnloaded() && assetHandle.GetAssetID().IsValid());
        m_pEditorWindowContext->m_pAssetService->Load(assetHandle);
    }

    void IEditorWindow::UnloadAsset(IAssetHandle& assetHandle)
    {
        assert(assetHandle.IsLoaded() && assetHandle.GetAssetID().IsValid());
        m_pEditorWindowContext->m_pAssetService->Unload(assetHandle);
    }
} // namespace aln