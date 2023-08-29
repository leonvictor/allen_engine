#include "graph/passthrough_node.hpp"

namespace aln
{
ALN_REGISTER_ABSTRACT_IMPL_BEGIN(PassthroughRuntimeNode::Settings)
ALN_REFLECT_BASE(PoseRuntimeNode::Settings)
ALN_REFLECT_MEMBER(m_childNodeIdx)
ALN_REGISTER_IMPL_END()
}