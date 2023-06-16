#pragma once

#include <assets/asset_id.hpp>

#include <nlohmann/json.hpp>

namespace aln
{

class WorldEntity;
class Entity;
class TypeRegistryService;
class IAssetHandle;
class UpdateContext;
class AssetService;

class EditorWindowContext
{
    friend class Editor;
    friend class IEditorWindow;

    // TODO: Shouldnt be here
    friend class AnimationGraphEditor;
    friend class EntityInspector;

    // TODO: Specific to entities-related windows ?
    WorldEntity* m_pWorldEntity = nullptr;
    Entity* m_pSelectedEntity = nullptr;

    const TypeRegistryService* m_pTypeRegistryService = nullptr;
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
    bool m_isOpen = true;

    virtual void Initialize(EditorWindowContext* pEditorWindowContext);
    virtual void Shutdown();
    void LoadAsset(IAssetHandle& assetHandle);
    void UnloadAsset(IAssetHandle& assetHandle);

    void RequestAssetWindowCreation(const AssetID& id) { m_pEditorWindowContext->m_requestedAssetWindowsCreations.emplace_back(id); }
    void RequestAssetWindowDeletion(const AssetID& id) { m_pEditorWindowContext->m_requestedAssetWindowsDeletions.emplace_back(id); }

    // Entity specific ?
    void SetSelectedEntity(Entity* pEntity) { m_pEditorWindowContext->m_pSelectedEntity = pEntity; }
    Entity* GetSelectedEntity() const { return m_pEditorWindowContext->m_pSelectedEntity; }
    WorldEntity* GetWorldEntity() const { return m_pEditorWindowContext->m_pWorldEntity; }

  public:
    virtual void Update(const UpdateContext& context) = 0; // TODO: name ?

    // ------- State management
    // The editor's state is saved to disk and loaded back
    virtual void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) = 0;
    virtual void SaveState(nlohmann::json& json) const = 0;
};
} // namespace aln