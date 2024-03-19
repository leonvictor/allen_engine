#pragma once

#include "editor_window.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <assets/asset.hpp>
#include <assets/asset_id.hpp>
#include <entities/services/worlds_service.hpp>
#include <assets/asset_service.hpp>

namespace aln
{

struct PreviewSceneSettings;

/// @brief Abstract base class for all editor windows related to an asset type
class IAssetWorkspace : public IEditorWindow
{
    friend class Editor;
    friend class PreviewSceneSettingsWindow;

  private:
    AssetID m_id;
    WorldsService* m_pWorldsService = nullptr;
    AssetService* m_pAssetService = nullptr;

  protected:
    WorldEntity* m_pPreviewWorld = nullptr;

    // TODO: Find a better way to keep the hardcoded editor asset paths. Probably after AssetID is refactored to not use strings everywhere...
    static constexpr const char* PreviewSceneFloorMeshAssetFilepath = "assets/editor/floor/Floor_Plane.mesh";

  protected:
    virtual void Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile = true)
    {
        assert(id.IsValid());
        IEditorWindow::Initialize(pContext);

        m_pWorldsService = pContext->m_pWorldsService;
        m_pAssetService = pContext->m_pAssetService;

        m_id = id;
    }

    virtual void Shutdown()
    {
        IEditorWindow::Shutdown();
        m_id = AssetID();
    }

    // Preview world
    void CreatePreviewWorld()
    {
        assert(m_pPreviewWorld == nullptr);
        m_pPreviewWorld = m_pWorldsService->CreateWorld(false);
    }

    void DeletePreviewWorld()
    {
        assert(m_pPreviewWorld != nullptr);
        m_pWorldsService->DestroyWorld(m_pPreviewWorld);
        m_pPreviewWorld = nullptr;
    }

    bool HasPreviewWorld() const { return m_pPreviewWorld != nullptr; }

    void RequestAssetLoad(IAssetHandle& handle) const { m_pAssetService->Load(handle); }
    void RequestAssetUnload(IAssetHandle& handle) const { m_pAssetService->Unload(handle); }

    virtual void StartEditingScenePreviewSetting(const TypeEditedEventDetails& editingEventDetails)
    {
        assert(HasPreviewWorld());
    }

    virtual void EndEditingScenePreviewSetting(const TypeEditedEventDetails& editingEventDetails)
    {
        assert(HasPreviewWorld());
    }

    virtual PreviewSceneSettings* GetPreviewSceneSettings() { return nullptr; }

  public:
    const AssetID& GetID() { return m_id; }

    virtual void SaveState(JSON& json) const override = 0;
    virtual void LoadState(JSON& json, const TypeRegistryService* pTypeRegistryService) override = 0;
};

class IAssetWorkspacesFactory
{
  public:
    std::string m_assetEditorName;
    AssetTypeID m_supportedAssetType;
    bool m_canBeCreatedByUser;
    virtual IAssetWorkspace* CreateWorkspace() = 0;
};

/// @brief Maps asset types to window factories.
/// @todo Better name
class AssetEditorWindowsFactories
{
    friend class Editor;

    Vector<IAssetWorkspacesFactory*> m_factories;

    template <typename AssetType, typename FactoryType>
    void RegisterFactory(std::string_view name, bool canBeCreatedByUser)
    {
        static_assert(std::is_base_of_v<IAsset, AssetType>);
        static_assert(std::is_base_of_v<IAssetWorkspacesFactory, FactoryType>);

        auto& pFactory = m_factories.emplace_back(aln::New<FactoryType>());
        pFactory->m_assetEditorName = name;
        pFactory->m_supportedAssetType = AssetType::GetStaticAssetTypeID();
        pFactory->m_canBeCreatedByUser = canBeCreatedByUser;
    }

    IAssetWorkspacesFactory* FindFactory(const AssetTypeID& assetTypeID) const
    {
        for (auto pFactory : m_factories)
        {
            if (pFactory->m_supportedAssetType == assetTypeID)
            {
                return pFactory;
            }
        }
        return nullptr;
    }

  public:
    ~AssetEditorWindowsFactories()
    {
        for (auto pFactory : m_factories)
        {
            aln::Delete(pFactory);
        }
        m_factories.clear();
    }

    bool IsTypeRegistered(const AssetTypeID& assetTypeID) const { return FindFactory(assetTypeID) != nullptr; }

    /// @brief Create a new asset editor window for the specified type. The memory is owned by the caller.
    IAssetWorkspace* CreateWorkspace(const AssetTypeID& assetTypeID)
    {
        assert(assetTypeID.IsValid());
        auto pFactory = FindFactory(assetTypeID);
        if (pFactory != nullptr)
        {
            return pFactory->CreateWorkspace();
        }
        return nullptr; // TODO: Default ?
    }
};

#define ALN_ASSET_WORKSPACE_FACTORY(assetType, workspaceType)             \
    class assetType##EditorWindowFactory : public IAssetWorkspacesFactory \
    {                                                                     \
      public:                                                             \
        virtual IAssetWorkspace* CreateWorkspace() override final         \
        {                                                                 \
            return aln::New<workspaceType>();                             \
        }                                                                 \
    };

} // namespace aln