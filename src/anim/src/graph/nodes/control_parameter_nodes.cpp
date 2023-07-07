#include "graph/nodes/control_parameter_nodes.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, FloatControlParameterNode::Settings)
ALN_REFLECT_BASE(RuntimeGraphNode::Settings)
ALN_REGISTER_IMPL_END()

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, BoolControlParameterNode::Settings)
ALN_REFLECT_BASE(RuntimeGraphNode::Settings)
ALN_REGISTER_IMPL_END()

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, IDControlParameterNode::Settings)
ALN_REFLECT_BASE(RuntimeGraphNode::Settings)
ALN_REGISTER_IMPL_END()
}