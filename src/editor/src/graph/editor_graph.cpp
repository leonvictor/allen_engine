#include "graph/editor_graph.hpp"

#include "graph/editor_graph_node.hpp"

#include <config/path.h>
#include <reflection/services/type_registry_service.hpp>
#include <reflection/type_info.hpp>

#include <imnodes.h>

#include <assert.h>

namespace aln
{

EditorGraph::~EditorGraph()
{
    for (auto pNode : m_graphNodes)
    {
        pNode->Shutdown();
        aln::Delete(pNode);
    }

    if (m_pImNodesEditorContext != nullptr)
    {
        ImNodes::EditorContextFree(m_pImNodesEditorContext);
    }
}

void EditorGraph::SaveState(nlohmann::json& json) const
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

void EditorGraph::LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
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

        AddLink(pOutputNode->GetID(), pOutputNode->GetOutputPin(linkJson["output_pin"]).GetID(), pInputNode->GetID(), pInputNode->GetInputPin(linkJson["input_pin"]).GetID());
    }
}

void EditorGraph::Clear()
{
    m_pinLookupMap.clear();
    m_nodeLookupMap.clear();
    m_links.Clear();

    for (auto pNode : m_graphNodes)
    {
        aln::Delete(pNode);
    }

    m_graphNodes.clear();
}

uint32_t EditorGraph::GetNodeIndex(const UUID& nodeID) const
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

const EditorGraphNode* EditorGraph::GetNodeLinkedToInputPin(const UUID& inputPinID) const
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

const EditorGraphNode* EditorGraph::GetNodeLinkedToOutputPin(const UUID& outputPinID) const
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

void EditorGraph::AddGraphNode(EditorGraphNode* pNode)
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

void EditorGraph::RemoveGraphNode(const UUID& nodeID)
{
    assert(nodeID.IsValid());

    auto pNode = *std::find_if(m_graphNodes.begin(), m_graphNodes.end(), [&](auto pNode)
        { return pNode->GetID() == nodeID; });

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
    m_links.EraseIf([&](auto& link)
        { return link.m_pInputNode == pNode || link.m_pOutputNode == pNode; });

    // Actually remove the node from the graph
    std::erase(m_graphNodes, pNode);
    aln::Delete(pNode);

    SetDirty();
}

const Link* EditorGraph::GetLinkToPin(const UUID& pinID) const
{
    for (const auto& link : m_links)
    {
        if (link.m_inputPinID == pinID || link.m_outputPinID == pinID)
        {
            return &link;
        }
    }
    return nullptr;
}

void EditorGraph::AddLink(UUID startNodeID, UUID startPinID, UUID endNodeID, UUID endPinID)
{
    assert(startNodeID.IsValid() && startPinID.IsValid() && endNodeID.IsValid() && endPinID.IsValid());

    auto pInputPin = m_pinLookupMap[endPinID];
    auto pOutputPin = m_pinLookupMap[startPinID];

    assert(pInputPin->IsInput() && pOutputPin->IsOutput());

    // Ensure matching pin types
    if (pInputPin->GetValueType() != pOutputPin->GetValueType())
    {
        return;
    }

    // Remove existing links if necessary
    // Input pins only accept one link
    const auto pLink = GetLinkToPin(endPinID);
    if (pLink != nullptr && pLink->m_outputPinID != endPinID)
    {
        RemoveLink(pLink->m_id);
    }

    if (!pOutputPin->AllowsMultipleLinks())
    {
        const auto pLink = GetLinkToPin(startPinID);
        if (pLink != nullptr && pLink->m_outputPinID != endPinID)
        {
            RemoveLink(pLink->m_id);
        }
    }

    // Finally create the new link
    auto& link = m_links.EmplaceBack();
    link.m_pInputNode = m_nodeLookupMap[endNodeID];
    link.m_inputPinID = pInputPin->GetID();
    link.m_pOutputNode = m_nodeLookupMap[startNodeID];
    link.m_outputPinID = pOutputPin->GetID();

    SetDirty();
}

void EditorGraph::RemoveLink(const UUID& linkID)
{
    assert(linkID.IsValid());
    m_links.Erase(linkID);

    SetDirty();
}

void EditorGraph::AddDynamicInputPin(EditorGraphNode* pNode)
{
    const auto& pin = pNode->AddDynamicInputPin(pNode->DynamicInputPinValueType(), pNode->DynamicInputPinName());
    pNode->OnDynamicInputPinCreated(pin.GetID());
    // Refresh lookup maps since ptrs to pins may have been invalidated by a resize
    for (auto& pin : pNode->GetInputPins())
    {
        m_pinLookupMap[pin.GetID()] = &pin;
    }
}

void EditorGraph::RemoveDynamicInputPin(EditorGraphNode* pNode, const UUID& pinID)
{
    m_pinLookupMap.erase(pinID);
    pNode->RemoveDynamicInputPin(pinID);
}
} // namespace aln