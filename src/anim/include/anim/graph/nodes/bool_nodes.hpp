#pragma once

#include "../value_node.hpp"

/// @brief Logical operation nodes (and, or, not)
/// @todo Allow dynamic size for and/or
/// @todo Cache the value to avoid updating input nodes if its not necessary
namespace aln
{
class BoolAndRuntimeNode : public BoolValueNode
{
  private:
    BoolValueNode* m_pInputValueNode1 = nullptr;
    BoolValueNode* m_pInputValueNode2 = nullptr;

  public:
    class Settings : public BoolValueNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class BoolAndEditorNode;

      private:
        NodeIndex m_inputValueNode1Idx = InvalidIndex;
        NodeIndex m_inputValueNode2Idx = InvalidIndex;

        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<BoolAndRuntimeNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_inputValueNode1Idx, pNode->m_pInputValueNode1);
            SetNodePtrFromIndex(nodePtrs, m_inputValueNode2Idx, pNode->m_pInputValueNode2);
        }
    };

    virtual void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        assert(context.IsValid());
        
        const auto inputValue1 = m_pInputValueNode1->GetValue<bool>(context);
        const auto inputValue2 = m_pInputValueNode2->GetValue<bool>(context);
        *((bool*) pValue) = inputValue1 && inputValue2;
    }

    virtual void InitializeInternal(GraphContext& context) override
    {
        BoolValueNode::InitializeInternal(context);
        m_pInputValueNode1->Initialize(context);
        m_pInputValueNode2->Initialize(context);
    }

    virtual void ShutdownInternal() override
    {
        m_pInputValueNode1->Shutdown();
        m_pInputValueNode2->Shutdown();
        BoolValueNode::ShutdownInternal();
    }
};

class BoolOrRuntimeNode : public BoolValueNode
{
  private:
    BoolValueNode* m_pInputValueNode1 = nullptr;
    BoolValueNode* m_pInputValueNode2 = nullptr;

  public:
    class Settings : public BoolValueNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class BoolOrEditorNode;

      private:
        NodeIndex m_inputValueNode1Idx = InvalidIndex;
        NodeIndex m_inputValueNode2Idx = InvalidIndex;

        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<BoolOrRuntimeNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_inputValueNode1Idx, pNode->m_pInputValueNode1);
            SetNodePtrFromIndex(nodePtrs, m_inputValueNode2Idx, pNode->m_pInputValueNode2);
        }
    };

    virtual void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        assert(context.IsValid());

        const auto inputValue1 = m_pInputValueNode1->GetValue<bool>(context);
        const auto inputValue2 = m_pInputValueNode2->GetValue<bool>(context);
        *((bool*) pValue) = inputValue1 || inputValue2;
    }

    virtual void InitializeInternal(GraphContext& context) override
    {
        BoolValueNode::InitializeInternal(context);
        m_pInputValueNode1->Initialize(context);
        m_pInputValueNode2->Initialize(context);
    }

    virtual void ShutdownInternal() override
    {
        m_pInputValueNode1->Shutdown();
        m_pInputValueNode2->Shutdown();
        BoolValueNode::ShutdownInternal();
    }
};

class BoolNotRuntimeNode : public BoolValueNode
{
  private:
    BoolValueNode* m_pInputValueNode = nullptr;

  public:
    class Settings : public BoolValueNode::Settings
    {
        ALN_REGISTER_TYPE();

        friend class BoolNotEditorNode;

      private:
        NodeIndex m_inputValueNodeIdx = InvalidIndex;

        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<BoolNotRuntimeNode>(nodePtrs, options);
            SetNodePtrFromIndex(nodePtrs, m_inputValueNodeIdx, pNode->m_pInputValueNode);
        }
    };

    virtual void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        assert(context.IsValid());

        const auto inputValue = m_pInputValueNode->GetValue<bool>(context);
        *((bool*) pValue) = !inputValue;
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