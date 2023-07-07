#pragma once

#include "../value_node.hpp"

namespace aln
{
class FloatControlParameterNode : public FloatValueNode
{
  private:
    float m_value = 0.0f;

  public:
    class Settings : public FloatValueNode::Settings
    {
        ALN_REGISTER_TYPE();

        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<FloatControlParameterNode>(nodePtrs, options);
        }
    };

    virtual void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        *((float*) pValue) = m_value;
    }

    virtual void SetValueInternal(GraphContext& context, void const* pValue) override
    {
        m_value = *((float*) pValue);
    }
};

class BoolControlParameterNode : public BoolValueNode
{
  private:
    bool m_value = false;

  public:
    class Settings : public BoolValueNode::Settings
    {
        ALN_REGISTER_TYPE();

        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<BoolControlParameterNode>(nodePtrs, options);
        }
    };

    virtual void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        *((bool*) pValue) = m_value;
    }

    virtual void SetValueInternal(GraphContext& context, void const* pValue) override
    {
        m_value = *((bool*) pValue);
    }
};

class IDControlParameterNode : public IDValueNode
{
  private:
    StringID m_value = StringID::InvalidID;

  public:
    class Settings : public IDValueNode::Settings
    {
        ALN_REGISTER_TYPE();

        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const override
        {
            auto pNode = CreateNode<IDControlParameterNode>(nodePtrs, options);
        }
    };

    virtual void GetValueInternal(GraphContext& context, void* pValue) const override
    {
        *((StringID*) pValue) = m_value;
    }

    virtual void SetValueInternal(GraphContext& context, void const* pValue) override
    {
        m_value = *((StringID*) pValue);
    }
};

// TODO: Implement other parameter types

} // namespace aln