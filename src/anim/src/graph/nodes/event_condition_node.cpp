#include "graph/nodes/event_condition_node.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, EventConditionRuntimeNode::Settings)
ALN_REFLECT_BASE(BoolValueNode::Settings)
ALN_REFLECT_MEMBER(m_eventID)
ALN_REGISTER_IMPL_END()
}