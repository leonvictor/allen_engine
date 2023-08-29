#include "graph/nodes/animation_clip_node.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_NODES_SETTINGS, AnimationClipRuntimeNode::Settings)
ALN_REFLECT_BASE(RuntimeGraphNode::Settings)
ALN_REFLECT_MEMBER(m_dataSlotIdx)
ALN_REFLECT_MEMBER(m_playInReverseValueNodeIdx)
ALN_REGISTER_IMPL_END()
} // namespace aln