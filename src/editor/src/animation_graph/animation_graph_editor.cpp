#include "animation_graph/animation_graph_editor.hpp"

#include "animation_graph/animation_graph_compilation_context.hpp"
#include "animation_graph/editor_graph_node.hpp"
#include "animation_graph/nodes/animation_clip_editor_node.hpp"
#include "animation_graph/nodes/blend_editor_node.hpp"
#include "animation_graph/nodes/control_parameter_editor_nodes.hpp"
#include "animation_graph/nodes/pose_editor_node.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <config/path.h>

#include <anim/graph/animation_graph_dataset.hpp>
#include <assets/asset_archive_header.hpp>
#include <entities/update_context.hpp>
#include <reflection/services/type_registry_service.hpp>
#include <reflection/type_info.hpp>

#include <imnodes.h>

#include <assert.h>

namespace aln
{

// Helpers
RGBColor GetTypeColor(NodeValueType valueType)
{
    // TODO: Handle all possible types
    // TODO: Handle hue variation on selected / hovered
    // TODO: Decide on a cool color palette
    switch (valueType)
    {
    case NodeValueType::Pose:
        return RGBColor::Pink;
    case NodeValueType::Float:
        return RGBColor::Yellow;
    default:
        assert(false); // Is the value type handled ?
        return RGBColor::Black;
    }
}

AnimationGraphDefinition* AnimationGraphEditor::Compile()
{
    assert(m_pTypeRegistryService != nullptr);

    AnimationGraphCompilationContext context(this);

    // Compile graph definition
    AnimationGraphDefinition graphDefinition;

    // Parameter nodes are compiled first to be easier to find
    auto parameterNodes = GetAllNodesOfType<IControlParameterEditorNode>();
    graphDefinition.m_controlParameterNames.reserve(parameterNodes.size());
    for (auto& pParameterNode : parameterNodes)
    {
        pParameterNode->Compile(context, &graphDefinition);
        graphDefinition.m_controlParameterNames.push_back(pParameterNode->GetParameterName());
    }

    auto outputNodes = GetAllNodesOfType<PoseEditorNode>();
    assert(outputNodes.size() == 1);

    auto rootNodeIndex = outputNodes[0]->Compile(context, &graphDefinition);
    graphDefinition.m_rootNodeIndex = rootNodeIndex;
    graphDefinition.m_requiredMemoryAlignement = context.GetNodeMemoryAlignement();
    graphDefinition.m_requiredMemorySize = context.GetNodeMemoryOffset();

    // Compile dataset
    AnimationGraphDataset graphDataset;
    auto& registeredDataSlots = context.GetRegisteredDataSlots();
    for (auto& slotOwnerNodeID : registeredDataSlots)
    {
        auto pOwnerNode = (AnimationClipEditorNode*) m_nodeLookupMap[slotOwnerNodeID];
        auto& clipID = pOwnerNode->GetAnimationClipID();
        assert(clipID.IsValid()); // TODO: Proper input validation
        graphDataset.m_animationClips.emplace_back(clipID);
    }

    // TODO: Where does runtime asset serialization+saving occur ?

    // Serialize graph definition
    {
        std::vector<std::byte> data;
        auto dataArchive = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);

        reflect::TypeCollectionDescriptor typeCollectionDesc;
        for (auto& pSettings : graphDefinition.m_nodeSettings)
        {
            auto pSettingsTypeInfo = pSettings->GetTypeInfo();
            auto& descriptor = typeCollectionDesc.m_descriptors.emplace_back();
            descriptor.DescribeTypeInstance(pSettings, m_pTypeRegistryService, pSettingsTypeInfo);
        }

        dataArchive << typeCollectionDesc;
        dataArchive << graphDefinition.m_nodeIndices;
        dataArchive << graphDefinition.m_rootNodeIndex;
        dataArchive << graphDefinition.m_nodeOffsets;
        dataArchive << graphDefinition.m_requiredMemorySize;
        dataArchive << graphDefinition.m_requiredMemoryAlignement;

        auto header = AssetArchiveHeader(AnimationGraphDefinition::GetStaticAssetTypeID());

        auto fileArchive = BinaryFileArchive(m_compiledDefinitionPath, IBinaryArchive::IOMode::Write);
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

        auto fileArchive = BinaryFileArchive(m_compiledDatasetPath, IBinaryArchive::IOMode::Write);
        fileArchive << header << data;
    }
    return nullptr;
}

void AnimationGraphEditor::SaveState(nlohmann::json& json)
{
    auto& nodes = json["nodes"];
    for (auto pNode : m_graphNodes)
    {
        auto& nodeJson = nodes.emplace_back();
        nodeJson["type"] = pNode->GetTypeInfo()->GetTypeID().GetHash();

        auto position = ImNodes::GetNodeEditorSpacePos(pNode->GetID());
        nodeJson["position"] = {position.x, position.y};

        pNode->SaveNodeState(nodeJson);
    }

    auto& links = json["links"];
    for (auto& link : m_links)
    {
        auto& linkJson = links.emplace_back();
        linkJson["input_node"] = GetNodeIndex(link.m_pInputNode->GetID());
        linkJson["input_pin"] = link.m_pInputNode->GetInputPinIndex(link.m_inputPinID);
        linkJson["output_node"] = GetNodeIndex(link.m_pOutputNode->GetID());
        linkJson["output_pin"] = link.m_pOutputNode->GetOutputPinIndex(link.m_outputPinID);
    }
}

void AnimationGraphEditor::LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
{
    assert(pTypeRegistryService != nullptr);

    for (const auto& nodeJson : json["nodes"])
    {
        uint32_t typeID = nodeJson["type"];
        auto pTypeInfo = pTypeRegistryService->GetTypeInfo(typeID);

        auto pNode = pTypeInfo->CreateTypeInstance<EditorGraphNode>();
        pNode->Initialize();
        pNode->LoadNodeState(nodeJson, pTypeRegistryService);

        AddGraphNode(pNode);

        ImNodes::SetNodeEditorSpacePos(pNode->GetID(), {nodeJson["position"][0], nodeJson["position"][1]});
    }

    for (const auto& linkJson : json["links"])
    {
        auto pInputNode = m_graphNodes[linkJson["input_node"]];
        auto pOutputNode = m_graphNodes[linkJson["output_node"]];

        AddLink(pInputNode->GetID(), pInputNode->GetInputPin(linkJson["input_pin"]).GetID(), pOutputNode->GetID(), pOutputNode->GetOutputPin(linkJson["output_pin"]).GetID());
    }
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

    if (ImGui::Begin(windowName.c_str(), &m_isOpen))
    {
        UUID hoveredNodeID, hoveredPinID, hoveredLinkID;
        bool nodeHovered = ImNodes::IsNodeHovered(&hoveredNodeID);
        bool pinHovered = ImNodes::IsPinHovered(&hoveredPinID);
        bool linkHovered = ImNodes::IsLinkHovered(&hoveredLinkID);

        ImNodes::BeginNodeEditor();
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick | ImNodesAttributeFlags_EnableLinkCreationOnSnap);

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

                // TODO: Save to a common folder with the rest of the editor ?
                nlohmann::json json;
                SaveState(json);
                std::ofstream outputStream(m_statePath);
                outputStream << json;
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
                ImNodes::PushColorStyle(ImNodesCol_Pin, GetTypeColor(inputPin.GetValueType()).U32());

                ImNodes::BeginInputAttribute(inputPin.GetID());
                ImGui::Text(inputPin.GetName().c_str());
                ImNodes::EndInputAttribute();

                ImNodes::PopColorStyle();
            }

            ImGui::Spacing();

            // Display reflected fields
            auto pTypeInfo = pNode->GetTypeInfo();
            m_nodeInspector.Draw(pTypeInfo, pNode, 100);

            ImGui::Spacing();

            for (auto& outputPin : pNode->m_outputPins)
            {
                ImNodes::PushColorStyle(ImNodesCol_Pin, GetTypeColor(outputPin.GetValueType()).U32());

                ImNodes::BeginOutputAttribute(outputPin.GetID());
                ImGui::Text(outputPin.GetName().c_str());
                ImNodes::EndOutputAttribute();

                ImNodes::PopColorStyle();
            }

            ImNodes::EndNode();
        }

        // Draw links
        for (auto& link : m_links)
        {
            const auto pPin = m_pinLookupMap[link.m_inputPinID];

            ImNodes::PushColorStyle(ImNodesCol_Link, GetTypeColor(pPin->GetValueType()).U32());
            ImNodes::Link(link.m_id, link.m_inputPinID, link.m_outputPinID);
            ImNodes::PopColorStyle();
        }

        ImNodes::PopAttributeFlag();
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
        bool createdFromSnap = false;
        if (ImNodes::IsLinkCreated(&startNodeID, &startPinID, &endNodeID, &endPinID, &createdFromSnap))
        {
            if (!createdFromSnap)
            {
                // Delete other links
                auto pInputPin = m_pinLookupMap[startPinID];
                if (!pInputPin->AllowsMultipleLinks())
                {
                    const auto& existingLinkID = GetLinkToPin(startPinID);
                    if (existingLinkID.IsValid())
                    {
                        RemoveLink(existingLinkID);
                    }
                }

                auto pOutputPin = m_pinLookupMap[endPinID];
                if (!pOutputPin->AllowsMultipleLinks())
                {
                    const auto& existingLinkID = GetLinkToPin(endPinID);
                    if (existingLinkID.IsValid())
                    {
                        RemoveLink(existingLinkID);
                    }
                }
            }
            AddLink(startNodeID, startPinID, endNodeID, endPinID);
        }

        UUID linkID;
        if (ImNodes::IsLinkDestroyed(&linkID))
        {
            RemoveLink(linkID);
        }
    }
    ImGui::End();

    if (!m_isOpen)
    {
        // TODO: Ensure we've saved
        RequestAssetWindowDeletion(GetID());
    }
}

void AnimationGraphEditor::Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile)
{
    // TODO: Which parts of this could be shared behavior with a parent class ?
    IAssetEditorWindow::Initialize(pContext, id);

    m_pTypeRegistryService = pContext->m_pTypeRegistryService;

    // TODO: Shared behavior with other asset windows ?
    m_compiledDefinitionPath = std::filesystem::path(id.GetAssetPath());

    m_compiledDatasetPath = m_compiledDefinitionPath;
    m_compiledDatasetPath.replace_extension(AnimationGraphDataset::GetStaticAssetTypeID().ToString());

    // Generate state path from id
    // TODO: state is saved in a separate directory
    m_statePath = m_compiledDefinitionPath;
    m_statePath.replace_filename(".json");

    if (readAssetFile)
    {
        if (std::filesystem::exists(m_statePath))
        {
            // TODO: Weird API
            std::ifstream inputStream(m_statePath);
            nlohmann::json json;
            inputStream >> json;

            LoadState(json, pContext->m_pTypeRegistryService);
        }
        else
        {
            // TODO: Load from compiled definition
        }
    }
}

void AnimationGraphEditor::Shutdown()
{
    if (IsDirty() && !m_statePath.empty())
    {
        nlohmann::json json;
        SaveState(json);
        std::ofstream outputStream(m_statePath);
        outputStream << json;
    }

    Clear();
    IAssetEditorWindow::Shutdown();
}

void AnimationGraphEditor::Clear()
{
    m_pinLookupMap.clear();
    m_links.clear();

    for (auto pNode : m_graphNodes)
    {
        aln::Delete(pNode);
    }

    m_graphNodes.clear();
    m_nodeLookupMap.clear();
}

uint32_t AnimationGraphEditor::GetNodeIndex(const UUID& nodeID)
{
    uint32_t nodeCount = m_graphNodes.size();
    for (uint32_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
    {
        auto pNode = m_graphNodes[nodeIndex];
        if (pNode->GetID() == nodeID)
        {
            return nodeIndex;
        }
    }
    return InvalidIndex;
}

const EditorGraphNode* AnimationGraphEditor::GetNodeLinkedToInputPin(const UUID& inputPinID) const
{
    for (auto& link : m_links)
    {
        if (link.m_inputPinID == inputPinID)
        {
            return link.m_pOutputNode;
        }
    }
    return nullptr;
}

const EditorGraphNode* AnimationGraphEditor::GetNodeLinkedToOutputPin(const UUID& outputPinID) const
{
    for (auto& link : m_links)
    {
        if (link.m_outputPinID == outputPinID)
        {
            return link.m_pInputNode;
        }
    }
    return nullptr;
}

void AnimationGraphEditor::AddGraphNode(EditorGraphNode* pNode)
{
    assert(pNode != nullptr);

    m_graphNodes.push_back(pNode);

    // Populate lookup maps
    m_nodeLookupMap[pNode->GetID()] = pNode;
    for (auto& pin : pNode->m_inputPins)
    {
        m_pinLookupMap[pin.GetID()] = &pin;
    }
    for (auto& pin : pNode->m_outputPins)
    {
        m_pinLookupMap[pin.GetID()] = &pin;
    }

    SetDirty();
}

void AnimationGraphEditor::RemoveGraphNode(const UUID& nodeID)
{
    assert(nodeID.IsValid());

    auto pNode = m_nodeLookupMap.at(nodeID);

    // Clean up lookup maps
    m_nodeLookupMap.erase(pNode->GetID());
    for (auto& pin : pNode->m_inputPins)
    {
        m_pinLookupMap.erase(pin.GetID());
    }
    for (auto& pin : pNode->m_outputPins)
    {
        m_pinLookupMap.erase(pin.GetID());
    }

    // Remove the node's attached links
    std::erase_if(m_links, [&](auto& link)
        { return link.m_pInputNode == pNode || link.m_pOutputNode == pNode; });

    // TODO: Actually remove the node from the graph
    std::erase(m_graphNodes, pNode);
    aln::Delete(pNode);

    SetDirty();
}

const UUID& AnimationGraphEditor::GetLinkToPin(const UUID& pinID) const
{
    for (auto& link : m_links)
    {
        if (link.m_inputPinID == pinID || link.m_outputPinID == pinID)
        {
            return link.m_id;
        }
    }
    return UUID::InvalidID();
}

void AnimationGraphEditor::AddLink(UUID inputNodeID, UUID inputPinID, UUID outputNodeID, UUID outputPinID)
{
    assert(inputNodeID.IsValid() && inputPinID.IsValid() && outputNodeID.IsValid() && outputPinID.IsValid());

    auto pInputPin = m_pinLookupMap[inputPinID];
    auto pOutputPin = m_pinLookupMap[outputPinID];

    // Ensure matching pin types
    if (pInputPin->GetValueType() != pOutputPin->GetValueType())
    {
        return;
    }

    // Ensure pins are input/output and in the right order
    if (!pInputPin->IsInput())
    {
        std::swap(pInputPin, pOutputPin);
        std::swap(inputNodeID, outputNodeID);
    }

    assert(pInputPin->IsInput() && pOutputPin->IsOutput());

    auto& link = m_links.emplace_back();
    link.m_pInputNode = m_nodeLookupMap[inputNodeID];
    link.m_inputPinID = pInputPin->GetID();
    link.m_pOutputNode = m_nodeLookupMap[outputNodeID];
    link.m_outputPinID = pOutputPin->GetID();

    SetDirty();
}

void AnimationGraphEditor::RemoveLink(const UUID& linkID)
{
    assert(linkID.IsValid());

    std::erase_if(m_links, [&](Link& link)
        { return link.m_id == linkID; });

    SetDirty();
}
} // namespace aln