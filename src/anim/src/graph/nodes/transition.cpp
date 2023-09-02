#include "graph/nodes/transition.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, TransitionRuntimeNode::Settings)
ALN_REFLECT_BASE(PoseRuntimeNode::Settings)
ALN_REFLECT_MEMBER(m_endStateNodeIdx)
ALN_REFLECT_MEMBER(m_transitionDuration)
ALN_REGISTER_IMPL_END()
}