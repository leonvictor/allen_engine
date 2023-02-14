#pragma once

#include "types_editor.hpp"

#include <assets/asset_id.hpp>
#include <assets/asset_service.hpp>
#include <assets/handle.hpp>
#include <entities/update_context.hpp>

namespace aln
{

class EditorWindowContext
{
    friend class Editor;
    friend class IEditorWindow;

    const TypeEditorService* m_pTypeEditorService = nullptr;
    // TODO: should be const
    AssetService* m_pAssetService = nullptr;

    std::vector<AssetID> m_requestedAssetWindowsCreations;
    std::vector<AssetID> m_requestedAssetWindowsDeletions;
};

/// @brief Interface for all editor windows
class IEditorWindow
{
    friend class Editor;

  protected:
    EditorWindowContext* m_pEditorWindowContext = nullptr;

    void Initialize(EditorWindowContext* pEditorWindowContext)
    {
        m_pEditorWindowContext = pEditorWindowContext;
    }

    virtual void Shutdown()
    {
        m_pEditorWindowContext = nullptr;
    }

    void LoadAsset(IAssetHandle& assetHandle)
    {
        assert(assetHandle.IsUnloaded() && assetHandle.GetAssetID().IsValid());
        m_pEditorWindowContext->m_pAssetService->Load(assetHandle);
    }

    void UnloadAsset(IAssetHandle& assetHandle)
    {
        assert(assetHandle.IsLoaded() && assetHandle.GetAssetID().IsValid());
        m_pEditorWindowContext->m_pAssetService->Unload(assetHandle);
    }

    void RequestAssetWindowCreation(const AssetID& id)
    {
        m_pEditorWindowContext->m_requestedAssetWindowsCreations.emplace_back(id);
    }

    void RequestAssetWindowDeletion(const AssetID& id)
    {
        m_pEditorWindowContext->m_requestedAssetWindowsDeletions.emplace_back(id);
    }

    const TypeEditorService* GetTypeEditorService() const { return m_pEditorWindowContext->m_pTypeEditorService; }

  public:
    virtual void Update(const UpdateContext& context) = 0; // TODO: name ?
};
} // namespace aln