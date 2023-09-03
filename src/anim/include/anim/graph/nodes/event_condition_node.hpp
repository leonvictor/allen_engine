#pragma once

#include "../runtime_graph_node.hpp"
#include "../value_node.hpp"

namespace aln
{
/// @brief Check whether a (state) event has been sampled this frame
class EventConditionRuntimeNode : public BoolValueNode
{
  public:
    class Settings : public BoolValueNode::Settings
    {
        ALN_REGISTER_TYPE()

        friend class EventConditionEditorNode;
        friend class EventConditionRuntimeNode;

      private:
        StringID m_eventID = StringID::InvalidID;

        void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const  override
        {
            CreateNode<EventConditionRuntimeNode>(nodePtrs, options);
        };
    };

  private:
  public:
    void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        // TODO : Cache the value and only evaluate if something changed
        const auto pSettings = GetSettings<EventConditionRuntimeNode>();
        bool found = false;
        for (const auto& sampledEvent : context.m_sampledEventsBuffer)
        {
            if (sampledEvent.IsStateEvent() && sampledEvent.GetStateEventID() == pSettings->m_eventID)
            {
                found = true;
                break;
            }
        }
        *((bool*) pValue) = found;
    }
};
} // namespace aln