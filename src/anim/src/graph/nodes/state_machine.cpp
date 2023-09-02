#include "graph/nodes/state_machine.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, StateMachineRuntimeNode::Settings)
ALN_REFLECT_BASE(PoseRuntimeNode::Settings)
ALN_REFLECT_MEMBER(m_stateSettings)
ALN_REGISTER_IMPL_END()
} // namespace aln