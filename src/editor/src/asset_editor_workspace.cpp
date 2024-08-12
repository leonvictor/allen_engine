#include "asset_editor_workspace.hpp"

#include <assets/assets_settings.hpp>
#include <common/services/settings_registry_service.hpp>

namespace aln
{
void IAssetWorkspace::Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile)
{
    assert(id.IsValid());
    IEditorWindow::Initialize(pContext);

    m_pWorldsService = pContext->m_pWorldsService;
    m_pAssetService = pContext->m_pAssetService;

    auto pSettings = pContext->m_pSettingsRegistryService->GetSettings<AssetsSettings>();
    m_editorAssetsDirectory = pSettings->m_editorAssetsDirectory;

    m_id = id;
}
} // namespace aln