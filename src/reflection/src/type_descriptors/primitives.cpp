/// Descriptors for primitive types.
#include "reflection.hpp"

#include <string>

namespace aln::reflect
{
/// @brief  A type descriptor for int
struct TypeDescriptor_Int : TypeDescriptor
{
    TypeDescriptor_Int() : TypeDescriptor{std::type_index(typeid(int))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<int>()
{
    static TypeDescriptor_Int typeDesc;
    return &typeDesc;
}

/// @brief A type descriptor for float
struct TypeDescriptor_Float : TypeDescriptor
{
    TypeDescriptor_Float() : TypeDescriptor{std::type_index(typeid(float))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<float>()
{
    static TypeDescriptor_Float typeDesc;
    return &typeDesc;
}

/// @brief A type descriptor for bool
struct TypeDescriptor_Bool : TypeDescriptor
{
    TypeDescriptor_Bool() : TypeDescriptor{std::type_index(typeid(bool))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<bool>()
{
    static TypeDescriptor_Bool typeDesc;
    return &typeDesc;
}
} // namespace aln::reflect
