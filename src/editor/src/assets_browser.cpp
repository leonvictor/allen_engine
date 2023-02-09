#include "assets_browser.hpp"

namespace aln
{

// TODO: This should be infered automatically from registered asset types
const std::vector<std::string> AssetsBrowser::AssetExtensionsFilter = {".anim", ".skel", ".mesh", ".smsh", ".text", ".agdf", ".agds"};

void AssetsBrowser::RecursiveDrawDirectory(const std::filesystem::directory_entry& directoryEntry)
{
    if (directoryEntry.is_directory())
    {
        auto nodeID = directoryEntry.path().filename().string() + "##" + directoryEntry.path().stem().string();
        if (ImGui::TreeNodeEx(nodeID.c_str()))
        {
            for (auto& childEntry : std::filesystem::directory_iterator(directoryEntry))
            {
                RecursiveDrawDirectory(childEntry);
            }
            ImGui::TreePop();
        }
    }
    else
    {
        auto ext = directoryEntry.path().extension().string();
        auto it = std::find(AssetExtensionsFilter.begin(), AssetExtensionsFilter.end(), ext);
        if (it != AssetExtensionsFilter.end())
        {
            ImGui::TreeNodeEx(directoryEntry.path().filename().string().c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                AssetID id = AssetID(directoryEntry.path().string());
                RequestAssetWindowCreation(id);
            }
            if (ImGui::BeginDragDropSource())
            {
                // Dragged tooltip
                ImGui::Text(directoryEntry.path().string().c_str());

                m_draggedAssetID = AssetID(directoryEntry.path().string());
                ImGui::SetDragDropPayload("AssetID", &m_draggedAssetID, sizeof(AssetID));

                ImGui::EndDragDropSource();
            }
        }
    }
}

void AssetsBrowser::Update(const UpdateContext& context)
{
    ImGui::Begin("Assets Browser");
    ImGui::Text(m_currentFilePath.string().c_str());
    ImGui::Separator();

    for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentFilePath))
    {
        RecursiveDrawDirectory(directoryEntry);
    }

    ImGui::End();
}

} // namespace aln