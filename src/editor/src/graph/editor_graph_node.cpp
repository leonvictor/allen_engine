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
    assert(pChildGraph != nullptr);
    assert(m_pChildGraph == nullptr);
    assert(!pChildGraph->IsInitialized());

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

} // namespace aln