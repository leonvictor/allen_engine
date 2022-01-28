#pragma once

#include <assert.h>
#include <string>

#include <glm/vec3.hpp>

#include <common/transform.hpp>

#include "../bone_mask.hpp"
#include "../sync_track.hpp"
#include "graph_node.hpp"

namespace aln
{

using StringID = std::string; // todo: tmp

template <typename T>
struct ValueTypeValidation
{
    static NodeValueType const Type = NodeValueType::Unknown;
};
template <>
struct ValueTypeValidation<bool>
{
    static NodeValueType const Type = NodeValueType::Bool;
};
template <>
struct ValueTypeValidation<StringID>
{
    static NodeValueType const Type = NodeValueType::ID;
};
template <>
struct ValueTypeValidation<uint32_t>
{
    static NodeValueType const Type = NodeValueType::Int;
};
template <>
struct ValueTypeValidation<float>
{
    static NodeValueType const Type = NodeValueType::Float;
};
template <>
struct ValueTypeValidation<glm::vec3>
{
    static NodeValueType const Type = NodeValueType::Vector;
};
template <>
struct ValueTypeValidation<Target>
{
    static NodeValueType const Type = NodeValueType::Target;
};
template <>
struct ValueTypeValidation<BoneMask>
{
    static NodeValueType const Type = NodeValueType::BoneMask;
};

/// @brief Animation graph node to compute values (ie int, floats, etc.)
/// @todo Value is lazily calculated then cached
/// @todo Value types:
/// - Bool, StringID, Int, Float, Vector, Target, BoneMask
class ValueNode : public GraphNode
{
  public:
    template <typename T>
    inline T GetValue(GraphContext& context)
    {
        assert(ValueTypeValidation<T>::Type == GetValueType());
        T value;
        GetValueInternal(context, &value);
        return value;
    }

    template <typename T>
    inline void SetValue(GraphContext& context, const T& value)
    {
        assert(ValueTypeValidation<T>::Type == GetValueType());
        SetValueInternal(context, &value);
    }

  protected:
    virtual void GetValueInternal(GraphContext& context, void* pValue) = 0;
    virtual void SetValueInternal(GraphContext& context, void const* pValue) = 0;
    virtual NodeValueType GetValueType() = 0;
};
} // namespace aln