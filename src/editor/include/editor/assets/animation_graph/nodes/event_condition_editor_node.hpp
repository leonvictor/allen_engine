#pragma once

#include "assets/animation_graph/animation_graph_compilation_context.hpp"
#include "graph/editor_graph_node.hpp"

#include <anim/graph/nodes/event_condition_node.hpp>

namespace aln
{
class EventConditionEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

  private:
    // Simple string, converted to stringID at compile time. This way we can still serialize it
    std::string m_eventID;

  protected:
    virtual void SaveState(nlohmann::json& json) const override
    {
        json["event_id"] = m_eventID;
    }

    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) override
    {
        m_eventID = json["event_id"];
    }

  public:
    void Initialize() override
    {
        m_name = "Event Condition";
        AddOutputPin(NodeValueType::Bool, "Result");
    }

    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
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
};
} // namespace aln