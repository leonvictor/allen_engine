#include "assets/animation_graph/nodes/event_condition_editor_node.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <anim/graph/nodes/event_condition_node.hpp>

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, EventConditionEditorNode)
ALN_REFLECT_BASE(EditorAnimationGraphNode)
ALN_REFLECT_MEMBER(m_eventID)
ALN_REGISTER_IMPL_END()

void EventConditionEditorNode::SaveState(JSON& json) const
{
    json["event_id"] = m_eventID;
}

void EventConditionEditorNode::LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService)
{
    m_eventID = json["event_id"];
}

void EventConditionEditorNode::Initialize()
{
    m_name = "Event Condition";
    AddOutputPin(NodeValueType::Bool, "Result");
}

NodeIndex EventConditionEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    EventConditionRuntimeNode::Settings* pSettings = nullptr;
    if (!context.GetSettings<EventConditionRuntimeNode>(this, graphDefinition, pSettings))
    {
        auto eventID = StringID(m_eventID);
        if (!eventID.IsValid())
        {
            context.LogError("Condition Event ID was invalid.");
            return InvalidIndex;
        }

        pSettings->m_eventID = eventID;
    }
    return pSettings->GetNodeIndex();
}
} // namespace aln