#pragma once

#include "../value_node.hpp"

namespace aln
{
class FloatClampRuntimeNode : public FloatValueNode
{
  private:
    ValueNode* m_pInputValueNode = nullptr;
    // Min and max could also be wired to ValueNodes
    float m_min = 0.0f;
    float m_max = 1.0f;

  public:
    class Settings : public FloatValueNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class FloatClampEditorNode;
      
    private:
        NodeIndex m_inputValueNodeIdx = InvalidIndex;
        float m_min = 0.0f;
        float m_max = 0.0f;

        virtual void InstanciateNode(const Vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<FloatClampRuntimeNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_inputValueNodeIdx, pNode->m_pInputValueNode);
            pNode->m_min = m_min;
            pNode->m_max = m_max;
        }
    };

    virtual void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        assert(context.IsValid());

        // TODO: We should avoid doing that if values weren't updated
        const auto pSettings = GetSettings<FloatClampRuntimeNode>();
        const auto inputValue = m_pInputValueNode->GetValue<float>(context);

        *((float*) pValue) = Maths::Clamp(inputValue, m_min, m_max);
    }

    virtual void InitializeInternal(GraphContext& context) override
    {
        FloatValueNode::InitializeInternal(context);
        m_pInputValueNode->Initialize(context);
    }

    virtual void ShutdownInternal() override
    {
        m_pInputValueNode->Shutdown();
        FloatValueNode::ShutdownInternal();
    }
};
} // namespace aln