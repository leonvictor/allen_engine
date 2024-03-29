#pragma once

#include "editor_window.hpp"

#include "assets/asset_id.hpp"
#include "assets/asset.hpp"

namespace aln
{
/// @brief Abstract base class for all editor windows related to an asset type
class IAssetWorkspace : public IEditorWindow
{
    friend class Editor;
    AssetID m_id;

  protected:
    virtual void Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile = true)
    {
        assert(id.IsValid());
        IEditorWindow::Initialize(pContext);
        m_id = id;
    }

    virtual void Shutdown()
    {
        IEditorWindow::Shutdown();
        m_id = AssetID();
    }

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
    virtual IAssetWorkspace* CreateWorkspace() = 0;
};

/// @brief Maps asset types to window factories.
/// @todo Better name
class AssetEditorWindowsFactories
{
    friend class Editor;

    Vector<IAssetWorkspacesFactory*> m_factories;

    template <typename AssetType, typename FactoryType>
    void RegisterFactory(std::string_view name)
    {
        static_assert(std::is_base_of_v<IAsset, AssetType>);
        static_assert(std::is_base_of_v<IAssetWorkspacesFactory, FactoryType>);

        auto& pFactory = m_factories.emplace_back(aln::New<FactoryType>());
        pFactory->m_assetEditorName = name;
        pFactory->m_supportedAssetType = AssetType::GetStaticAssetTypeID();
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

#define ALN_ASSET_WORKSPACE_FACTORY(assetType, workspaceType)                      \
    class assetType##EditorWindowFactory : public IAssetWorkspacesFactory \
    {                                                                        \
      public:                                                                \
        virtual IAssetWorkspace* CreateWorkspace() override final      \
        {                                                                    \
            return aln::New<workspaceType>();                                   \
        }                                                                    \
    };

} // namespace aln