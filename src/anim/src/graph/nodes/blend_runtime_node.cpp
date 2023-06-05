#include "graph/nodes/blend_node.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, BlendNode::Settings)
ALN_REFLECT_BASE(PoseRuntimeNode::Settings)
ALN_REFLECT_MEMBER(m_blendWeightValueNodeIdx)
ALN_REFLECT_MEMBER(m_sourcePoseNode1Idx)
ALN_REFLECT_MEMBER(m_sourcePoseNode2Idx)
ALN_REGISTER_IMPL_END()
}