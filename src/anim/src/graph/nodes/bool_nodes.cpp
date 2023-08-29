#include "graph/nodes/bool_nodes.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, BoolAndRuntimeNode::Settings)
ALN_REFLECT_BASE(BoolValueNode::Settings)
ALN_REFLECT_MEMBER(m_inputValueNode1Idx)
ALN_REFLECT_MEMBER(m_inputValueNode2Idx)
ALN_REGISTER_IMPL_END()

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, BoolOrRuntimeNode::Settings)
ALN_REFLECT_BASE(BoolValueNode::Settings)
ALN_REFLECT_MEMBER(m_inputValueNode1Idx)
ALN_REFLECT_MEMBER(m_inputValueNode2Idx)
ALN_REGISTER_IMPL_END()

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, BoolNotRuntimeNode::Settings)
ALN_REFLECT_BASE(BoolValueNode::Settings)
ALN_REFLECT_MEMBER(m_inputValueNodeIdx)
ALN_REGISTER_IMPL_END()
} // namespace aln