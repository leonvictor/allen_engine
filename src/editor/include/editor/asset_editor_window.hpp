#pragma once

#include "editor_window.hpp"

#include "assets/asset_id.hpp"
#include "assets/asset.hpp"

#include <map>

namespace aln
{
/// @brief Abstract base class for all editor windows related to an asset type
class IAssetEditorWindow : public IEditorWindow
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

    virtual void SaveState(nlohmann::json& json) override = 0;
    virtual void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override = 0;
};

class IAssetEditorWindowsFactory
{
  public:
    std::string m_assetEditorName;
    AssetTypeID m_supportedAssetType;
    virtual IAssetEditorWindow* CreateEditorWindow() = 0;
};

/// @brief Maps asset types to window factories.
/// @todo Better name
class AssetEditorWindowsFactories
{
    friend class Editor;

    std::vector<IAssetEditorWindowsFactory*> m_factories;

    template <typename AssetType, typename FactoryType>
    void RegisterFactory(std::string_view name)
    {
        static_assert(std::is_base_of_v<IAsset, AssetType>);
        static_assert(std::is_base_of_v<IAssetEditorWindowsFactory, FactoryType>);

        auto& pFactory = m_factories.emplace_back(aln::New<FactoryType>());
        pFactory->m_assetEditorName = name;
        pFactory->m_supportedAssetType = AssetType::GetStaticAssetTypeID();
    }

    IAssetEditorWindowsFactory* FindFactory(const AssetTypeID& assetTypeID) const
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
    IAssetEditorWindow* CreateEditorWindow(const AssetTypeID& assetTypeID)
    {
        assert(assetTypeID.IsValid());
        auto pFactory = FindFactory(assetTypeID);
        if (pFactory != nullptr)
        {
            return pFactory->CreateEditorWindow();
        }
        return nullptr; // TODO: Default ?
    }
};

#define ALN_ASSET_WINDOW_FACTORY(assetType, windowType)                      \
    class assetType##EditorWindowFactory : public IAssetEditorWindowsFactory \
    {                                                                        \
      public:                                                                \
        virtual IAssetEditorWindow* CreateEditorWindow() override final      \
        {                                                                    \
            return aln::New<windowType>();                                   \
        }                                                                    \
    };

} // namespace aln