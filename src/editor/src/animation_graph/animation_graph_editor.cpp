#include "animation_graph/animation_graph_editor.hpp"
#include "animation_graph/animation_graph_compilation_context.hpp"
#include "animation_graph/editor_graph_node.hpp"
#include "animation_graph/nodes/animation_clip_editor_node.hpp"
#include "animation_graph/nodes/pose_editor_node.hpp"

#include <anim/graph/animation_graph_dataset.hpp>

#include <assert.h>

namespace aln
{
AnimationGraphDefinition* AnimationGraphEditor::Compile()
{
    AnimationGraphCompilationContext context(this);

    // Compile the graph definition

    // Find the output node
    auto outputNodes = GetAllNodesOfType<PoseEditorNode>();
    assert(outputNodes.size() == 1);

    AnimationGraphDefinition graphDefinition;
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

    // TODO: At this point we have both a definition and a dataset so we're good to go !
    return nullptr;
}

void AnimationGraphEditor::Draw()

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
            auto& animGraphNodeTypes = aln::reflect::GetTypesInScope("ANIM_GRAPH_NODE");
            for (auto& pAnimGraphNodeType : animGraphNodeTypes)
            {
                if (ImGui::Selectable(pAnimGraphNodeType->GetPrettyName().c_str()))
                {
                    auto pNode = pAnimGraphNodeType->typeHelper->CreateType<EditorGraphNode>();
                    pNode->Initialize();

                    AddGraphNode(pNode);

                    ImNodes::SetNodeScreenSpacePos(pNode->GetID(), mousePos);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Debug: Compile Graph"))
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

        // Display reflected fields
        ImGui::PushItemWidth(100); // Max node width
        auto pTypeDesc = pNode->GetType();
        GetTypeEditorService()->DisplayTypeStruct(pTypeDesc, pNode);

        for (auto& outputPin : pNode->m_outputPins)
        {
            ImNodes::BeginOutputAttribute(outputPin.GetID());
            ImGui::Text(outputPin.GetName().c_str());
            ImNodes::EndOutputAttribute();
        }
        ImGui::PopItemWidth();

        ImNodes::EndNode();
    }

    // Draw links
    for (auto& link : m_links)
    {
        ImNodes::Link(link.m_id, link.m_inputPinID, link.m_outputPinID);
    }

    ImNodes::EndNodeEditor();

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

} // namespace aln