#pragma once

#include <imgui.h>

#include <assets/asset_id.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace aln::editor
{
/// @brief Folder browser for assets
/// @note This is pretty basic for now but it does the job.
class AssetsBrowser
{
  private:
    static const std::vector<std::string> AssetExtensionsFilter;

    std::filesystem::path m_currentFilePath;
    AssetID m_draggedAssetID;

    void RecursiveDrawDirectory(const std::filesystem::directory_entry& directoryEntry);

  public:
    AssetsBrowser(std::string folderPath) : m_currentFilePath(folderPath) {}
    void Draw();
};
} // namespace aln::editor