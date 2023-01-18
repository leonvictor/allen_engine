#pragma once

#include "editor_window.hpp"

#include "assets/asset_id.hpp"

#include <map>

namespace aln
{
/// @brief Abstract base class for all editor windows related to an asset type
class IAssetEditorWindow : public IEditorWindow
{
    friend class Editor;
    AssetID m_id;

  protected:
    virtual void Initialize(EditorWindowContext* pContext, const AssetID& id)
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
};

class IAssetEditorWindowsFactory
{
  public:
    virtual IAssetEditorWindow* CreateEditorWindow() = 0;
};

/// @brief Maps asset types to window factories.
/// @todo Better name
class AssetEditorWindowsFactories
{
    friend class Editor;

    std::map<AssetTypeID, IAssetEditorWindowsFactory*> m_factories;

    template <typename AssetType, typename FactoryType>
    void RegisterFactory()
    {
        static_assert(std::is_base_of_v<IAsset, AssetType>);
        static_assert(std::is_base_of_v<IAssetEditorWindowsFactory, FactoryType>);

        m_factories.try_emplace(AssetType::GetStaticAssetTypeID(), aln::New<FactoryType>());
    }

  public:
    ~AssetEditorWindowsFactories()
    {
        for (auto [id, pFactory] : m_factories)
        {
            aln::Delete(pFactory);
        }
        m_factories.clear();
    }

    bool IsTypeRegistered(const AssetTypeID& assetTypeID)
    {
        auto it = m_factories.find(assetTypeID);
        return it != m_factories.end();
    }

    /// @brief Create a new asset editor window for the specified type. The memory is owned by the caller.
    IAssetEditorWindow* CreateEditorWindow(const AssetTypeID& assetTypeID)
    {
        assert(assetTypeID.IsValid());

        auto it = m_factories.find(assetTypeID);
        if (it == m_factories.end())
        {
            return nullptr; // TODO: Default ?
        }
        return it->second->CreateEditorWindow();
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