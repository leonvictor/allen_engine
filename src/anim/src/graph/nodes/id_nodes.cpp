#include "graph/nodes/id_nodes.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, IDComparisonRuntimeNode::Settings)
ALN_REFLECT_BASE(BoolValueNode::Settings)
ALN_REFLECT_MEMBER(m_inputValueNodeIdx)
ALN_REFLECT_MEMBER(m_compareToID)
ALN_REGISTER_IMPL_END()
} // namespace aln