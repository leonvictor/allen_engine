#include "graph/editor_graph_node.hpp"

#include "graph/editor_graph.hpp"

namespace aln
{
ALN_REGISTER_ABSTRACT_IMPL_BEGIN(EditorGraphNode);
ALN_REGISTER_IMPL_END();

EditorGraphNode::~EditorGraphNode()
{
    assert(m_pChildGraph == nullptr);
}

void EditorGraphNode::SetChildGraph(EditorGraph* pChildGraph)
{
    assert(pChildGraph != nullptr && m_pChildGraph == nullptr);
    assert(!pChildGraph->IsInitialized());
    assert(m_pOwningGraph != nullptr);

    pChildGraph->Initialize(m_pOwningGraph);

    m_pChildGraph = pChildGraph;
}

 void EditorGraphNode::Shutdown()
{
    if (m_pChildGraph != nullptr)
    {
        m_pChildGraph->Shutdown();
        aln::Delete(m_pChildGraph);
        m_pChildGraph = nullptr;
    }
}

 void EditorGraphNode::SaveNodeState(nlohmann::json& json) const
{
    if (IsRenamable())
    {
        json["name"] = m_name;
    }

    SaveState(json);

    if (HasChildGraph())
    {
        auto& childGraphJson = json["child_graph"];
        childGraphJson["type"] = m_pChildGraph->GetTypeInfo()->GetTypeID().GetHash();
        m_pChildGraph->SaveState(childGraphJson);
    }
}

void EditorGraphNode::LoadNodeState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
{
    if (IsRenamable())
    {
        m_name = json["name"];
    }

    LoadState(json, pTypeRegistryService);

    if (json.contains("child_graph"))
    {
        assert(m_pOwningGraph != nullptr);

        const auto& childGraphJson = json["child_graph"];
        uint32_t typeID = childGraphJson["type"];
        const auto pTypeInfo = pTypeRegistryService->GetTypeInfo(typeID);

        m_pChildGraph = pTypeInfo->CreateTypeInstance<EditorGraph>();
        m_pChildGraph->Initialize(m_pOwningGraph);
        m_pChildGraph->LoadState(childGraphJson, pTypeRegistryService);
    }
}

} // namespace aln