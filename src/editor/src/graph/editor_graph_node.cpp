#include "graph/editor_graph_node.hpp"

#include "graph/editor_graph.hpp"

namespace aln
{
ALN_REGISTER_ABSTRACT_IMPL_BEGIN(EditorGraphNode);
ALN_REGISTER_IMPL_END();

EditorGraphNode::~EditorGraphNode()
{
    if (m_pChildGraph != nullptr)
    {
        aln::Delete(m_pChildGraph);
    }
}
} // namespace aln