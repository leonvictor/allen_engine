#include "graph/nodes/float_nodes.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, FloatClampRuntimeNode::Settings)
ALN_REFLECT_BASE(FloatValueNode::Settings)
ALN_REFLECT_MEMBER(m_inputValueNodeIdx)
ALN_REFLECT_MEMBER(m_min)
ALN_REFLECT_MEMBER(m_max)
ALN_REGISTER_IMPL_END()
} // namespace aln