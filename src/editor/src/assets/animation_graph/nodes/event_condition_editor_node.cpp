#include "assets/animation_graph/nodes/event_condition_editor_node.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, EventConditionEditorNode)
ALN_REFLECT_BASE(EditorAnimationGraphNode)
ALN_REFLECT_MEMBER(m_eventID)
ALN_REGISTER_IMPL_END()
}