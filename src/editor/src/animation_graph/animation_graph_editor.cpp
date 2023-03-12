#include "animation_graph/animation_graph_editor.hpp"
#include "animation_graph/animation_graph_compilation_context.hpp"
#include "animation_graph/editor_graph_node.hpp"
#include "animation_graph/nodes/animation_clip_editor_node.hpp"
#include "animation_graph/nodes/pose_editor_node.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <anim/graph/animation_graph_dataset.hpp>
#include <assets/asset_archive_header.hpp>
#include <assets/asset_service.hpp>
#include <config/path.h>
#include <core/asset_loaders/animation_graph_loader.hpp>
#include <reflection/services/type_registry_service.hpp>
#include <reflection/type_info.hpp>

#include <assert.h>

namespace aln
{
AnimationGraphDefinition* AnimationGraphEditor::Compile()
{
    AnimationGraphCompilationContext context(this);
    auto assetPath = std::filesystem::path(GetID().GetAssetPath());

    // Compile graph definition
    AnimationGraphDefinition graphDefinition;

    auto outputNodes = GetAllNodesOfType<PoseEditorNode>();
    assert(outputNodes.size() == 1);

    outputNodes[0]->Compile(context, &graphDefinition);

    // Compile dataset
    AnimationGraphDataset graphDataset;
    auto& registeredDataSlots = context.GetRegisteredDataSlots();
    for (auto& slotOwnerNodeID : registeredDataSlots)
    {
        auto pOwnerNode = (AnimationClipEditorNode*) m_nodeLookupMap[slotOwnerNodeID];
        auto& clipID = pOwnerNode->GetAnimationClipID();
        assert(clipID.IsValid());
        graphDataset.m_animationClips.emplace_back(clipID);
    }

    // TODO: Where does runtime asset serialization+saving occur ?

    // Serialize graph definition
    {
        std::vector<std::byte> data;
        auto dataArchive = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);
        graphDefinition.Serialize(dataArchive);

        auto header = AssetArchiveHeader(AnimationGraphDefinition::GetStaticAssetTypeID());

        // TODO: Path
        auto fileArchive = BinaryFileArchive(assetPath, IBinaryArchive::IOMode::Write);
        fileArchive << header << data;
    }

    // Serialize graph dataset
    {
        std::vector<std::byte> data;
        auto dataArchive = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);
        graphDataset.Serialize(dataArchive);

        auto header = AssetArchiveHeader(AnimationGraphDataset::GetStaticAssetTypeID());
        for (auto& handle : graphDataset.m_animationClips)
        {
            header.AddDependency(handle.GetAssetID());
        }

        // TODO: Path
        assetPath.replace_extension(AnimationGraphDataset::GetStaticAssetTypeID().ToString());
        auto fileArchive = BinaryFileArchive(assetPath, IBinaryArchive::IOMode::Write);
        fileArchive << header << data;
    }

    m_dirty = false;
    return nullptr;
}

void AnimationGraphEditor::Update(const UpdateContext& context)
{
    // TODO: It might be better to save that during initialization
    const auto pTypeRegistryService = context.GetService<TypeRegistryService>();

    std::string windowName = "Animation Graph";
    if (IsDirty())
    {
        // TODO: Modifying a window name changes its ID and thus it loses its position
        // windowName += "*";
    }

    if (m_waitingForAssetLoad)
    {
        if (m_pGraphDefinition.IsLoaded())
        {
            // TODO: Initialize the editor from loaded graph data
            m_waitingForAssetLoad = false;
        }
        else
        {
            // ImGui::Text("Waiting for asset load");
            // return; // TODO
        }
    }

    if (ImGui::Begin(windowName.c_str(), &m_shouldClose))
    {
        UUID hoveredNodeID, hoveredPinID, hoveredLinkID;
        bool nodeHovered = ImNodes::IsNodeHovered(&hoveredNodeID);
        bool pinHovered = ImNodes::IsPinHovered(&hoveredPinID);
        bool linkHovered = ImNodes::IsLinkHovered(&hoveredLinkID);

        ImNodes::BeginNodeEditor();

        // Open contextual popups and register context ID
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImNodes::IsEditorHovered())
        {
            if (pinHovered)
            {
                m_contextPopupElementID = hoveredPinID;
                ImGui::OpenPopup("graph_editor_pin_popup");
            }
            else if (nodeHovered)
            {
                m_contextPopupElementID = hoveredNodeID;
                ImGui::OpenPopup("graph_editor_node_popup");
            }
            else if (linkHovered)
            {
                m_contextPopupElementID = hoveredLinkID;
                ImGui::OpenPopup("graph_editor_link_popup");
            }
            else
            {
                m_contextPopupElementID = UUID::InvalidID();
                ImGui::OpenPopup("graph_editor_canvas_popup");
            }
        }

        if (ImGui::BeginPopup("graph_editor_canvas_popup"))
        {
            const ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();

            if (ImGui::BeginMenu("Add Node"))
            {
                auto& animGraphNodeTypes = pTypeRegistryService->GetTypesInScope("ANIM_GRAPH_EDITOR_NODES");
                for (auto& pAnimGraphNodeType : animGraphNodeTypes)
                {
                    if (ImGui::Selectable(pAnimGraphNodeType->m_name.c_str()))
                    {
                        auto pNode = pAnimGraphNodeType->CreateTypeInstance<EditorGraphNode>();
                        pNode->Initialize();

                        AddGraphNode(pNode);

                        ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Compile & Save"))
            {
                Compile();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_pin_popup"))
        {
            ImGui::MenuItem("Pin !");
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_node_popup"))
        {
            if (ImGui::MenuItem("Remove node"))
            {
                RemoveGraphNode(m_contextPopupElementID);
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("graph_editor_link_popup"))
        {
            if (ImGui::MenuItem("Remove link"))
            {
                RemoveLink(m_contextPopupElementID);
            }
            ImGui::EndPopup();
        }

        // Draw nodes
        for (auto pNode : m_graphNodes)
        {
            ImNodes::BeginNode(pNode->GetID());
            ImNodes::BeginNodeTitleBar();
            ImGui::Text(pNode->GetName().c_str());
            ImNodes::EndNodeTitleBar();

            for (auto& inputPin : pNode->m_inputPins)
            {
                ImNodes::BeginInputAttribute(inputPin.GetID());
                ImGui::Text(inputPin.GetName().c_str());
                ImNodes::EndInputAttribute();
            }

            ImGui::Spacing();

            // Display reflected fields
            auto pTypeInfo = pNode->GetTypeInfo();
            ReflectedTypeEditor::Draw(pTypeInfo, pNode, 100);

            ImGui::Spacing();

            for (auto& outputPin : pNode->m_outputPins)
            {
                ImNodes::BeginOutputAttribute(outputPin.GetID());
                ImGui::Text(outputPin.GetName().c_str());
                ImNodes::EndOutputAttribute();
            }

            ImNodes::EndNode();
        }

        // Draw links
        for (auto& link : m_links)
        {
            ImNodes::Link(link.m_id, link.m_inputPinID, link.m_outputPinID);
        }

        ImNodes::EndNodeEditor();

        // if (ImGui::BeginDragDropTarget())
        // {
        //     if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("AssetID", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        //     {
        //         assert(pPayload->DataSize == sizeof(AssetID));
        //         auto pID = (AssetID*) pPayload->Data;
        //         if (pID->GetAssetTypeID() == AnimationGraphDefinition::GetStaticAssetTypeID())
        //         {
        //             auto pAssetService = context.GetService<AssetService>();
        //             pAssetService->Load(m_)
        //         }
        //         if (pID->GetAssetTypeID() == AnimationGraphDataset::GetStaticAssetTypeID())
        //         {
        //         }
        //     }
        //     ImGui::EndDragDropTarget();
        // }

        // Handle link creation
        UUID startPinID, endPinID, startNodeID, endNodeID;
        if (ImNodes::IsLinkCreated(&startNodeID, &startPinID, &endNodeID, &endPinID))
        {
            AddLink(startNodeID, startPinID, endNodeID, endPinID);
        }

        UUID linkID;
        if (ImNodes::IsLinkDestroyed(&linkID))
        {
            RemoveLink(linkID);
        }
    }
    ImGui::End();

    if (m_shouldClose)
    {
        // TODO: Ensure we've saved
        RequestAssetWindowDeletion(GetID());
    }
}

} // namespace aln