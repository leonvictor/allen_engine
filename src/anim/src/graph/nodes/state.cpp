#include "graph/nodes/state.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, StateRuntimeNode::Settings);
ALN_REFLECT_BASE(PassthroughRuntimeNode::Settings)
ALN_REFLECT_MEMBER(m_entryEventID)
ALN_REFLECT_MEMBER(m_exitEventID)
ALN_REFLECT_MEMBER(m_inStateEventID)
ALN_REGISTER_IMPL_END()
}