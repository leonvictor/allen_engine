#pragma once

#include "../value_node.hpp"

namespace aln
{
/// @todo: Cache result
class IDComparisonRuntimeNode : public BoolValueNode
{
  private:
    IDValueNode* m_pInputValueNode = nullptr;

  public:
    class Settings : public BoolValueNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class IDComparisonEditorNode;

      private:
        NodeIndex m_inputValueNodeIdx = InvalidIndex;
        StringID m_compareToID = StringID::InvalidID;

        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<IDComparisonRuntimeNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_inputValueNodeIdx, pNode->m_pInputValueNode);
        }

      public:
        const StringID& GetCompareToID() const { return m_compareToID; }
    };

    virtual void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        assert(context.IsValid());

        // TODO: We should avoid doing that if values weren't updated
        const auto pSettings = GetSettings<IDComparisonRuntimeNode>();
        const auto inputValue = m_pInputValueNode->GetValue<StringID>(context);

        *((bool*) pValue) = (pSettings->GetCompareToID() == inputValue);
    }

    virtual void InitializeInternal(GraphContext& context) override
    {
        BoolValueNode::InitializeInternal(context);
        m_pInputValueNode->Initialize(context);
    }

    virtual void ShutdownInternal() override
    {
        m_pInputValueNode->Shutdown();
        BoolValueNode::ShutdownInternal();
    }
};
} // namespace aln