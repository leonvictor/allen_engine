#include "graph/runtime_graph_node.hpp"

namespace aln
{
ALN_REGISTER_ABSTRACT_IMPL_BEGIN(RuntimeGraphNode::Settings)
ALN_REFLECT_MEMBER(m_nodeIndex)
ALN_REGISTER_IMPL_END()
} // namespace aln