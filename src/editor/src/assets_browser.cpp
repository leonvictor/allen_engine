#include "assets_browser.hpp"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

namespace aln
{

// TODO: This should be infered automatically from registered asset types
const std::vector<std::string> AssetsBrowser::AssetExtensionsFilter = {".anim", ".skel", ".mesh", ".smsh", ".text", ".agdf", ".agds"};

void AssetsBrowser::RecursiveDrawDirectory(const std::filesystem::directory_entry& directoryEntry)
{
    if (directoryEntry.is_directory())
    {
        auto nodeID = directoryEntry.path().filename().string() + "##" + directoryEntry.path().stem().string();
        bool nodeOpen = ImGui::TreeNodeEx(nodeID.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("AssetID", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                assert(pPayload->DataSize == sizeof(AssetID));
                AssetID assetID = *((AssetID*) pPayload->Data);
                std::filesystem::path originalAssetPath = std::filesystem::path(assetID.GetAssetPath());
                std::filesystem::path newAssetPath = directoryEntry.path() / originalAssetPath.filename();

                // TODO: Better error handling
                assert(!std::filesystem::exists(newAssetPath));

                std::filesystem::rename(originalAssetPath, newAssetPath);
                // TODO: Update asset dependencies
            }

            // TODO: Also handle moving folder hierarchies

            ImGui::EndDragDropTarget();
        }

        if (nodeOpen)
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
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth;
            if (directoryEntry.path() == m_selectedAsset)
            {
                nodeFlags = nodeFlags |= ImGuiTreeNodeFlags_Selected;
            }

            // Renaming widget
            if (directoryEntry.path() == m_selectedAsset && m_renaming)
            {
                auto& style = ImGui::GetStyle();
                ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, style.IndentSpacing - style.FramePadding.x);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {style.FramePadding.x, 0});
                ImGui::Indent();

                auto inputName = m_selectedAsset.stem().string();
                if (ImGui::InputText("##rename", &inputName, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    // TODO: Validate user input
                    auto newAbsolutePath = m_selectedAsset;
                    newAbsolutePath.replace_filename(inputName);
                    newAbsolutePath.replace_extension(m_selectedAsset.extension());

                    if (std::filesystem::exists(newAbsolutePath))
                    {
                        assert(false); // TODO
                    }

                    std::filesystem::rename(m_selectedAsset, newAbsolutePath);

                    // TODO: Propagate change to all dependant assets

                    m_selectedAsset = newAbsolutePath;
                }

                ImGui::Unindent();
                ImGui::PopStyleVar();
                ImGui::PopStyleVar();
            }
            else
            {
                ImGui::TreeNodeEx(directoryEntry.path().filename().string().c_str(), nodeFlags);

                if (ImGui::IsItemClicked())
                {
                    m_selectedAsset = directoryEntry.path();
                    m_renaming = false;
                }

                if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    AssetID id = AssetID(directoryEntry.path().string());
                    RequestAssetWindowCreation(id);
                }

                if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
                {
                    m_renaming = false;
                    if (ImGui::MenuItem("Rename..."))
                    {
                        m_selectedAsset = directoryEntry.path();
                        m_renaming = true;
                    }
                    ImGui::EndPopup();
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