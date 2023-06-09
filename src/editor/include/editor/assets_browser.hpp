#pragma once

#include "editor_window.hpp"

#include <assets/asset_id.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace aln
{
/// @brief Folder browser for assets
/// @note This is pretty basic for now but it does the job.
class AssetsBrowser : public IEditorWindow
{
  private:
    static const std::vector<std::string> AssetExtensionsFilter;

    std::filesystem::path m_currentFilePath;
    AssetID m_draggedAssetID;

    std::filesystem::path m_selectedAsset = "";
    bool m_renaming = false;

    void RecursiveDrawDirectory(const std::filesystem::directory_entry& directoryEntry);

  public:
    AssetsBrowser(std::string folderPath) : m_currentFilePath(folderPath)
    {
        // TMP
        m_currentFilePath.make_preferred();
    }

    void Update(const UpdateContext& context) override;

    virtual void LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override{};
    virtual void SaveState(nlohmann::json& json) const override{};
};
} // namespace aln