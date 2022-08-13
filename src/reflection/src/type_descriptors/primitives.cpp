/// Descriptors for primitive types.
#include "reflection.hpp"

#include <string>

namespace aln::reflect
{
/// @brief  A type descriptor for int
struct TypeDescriptor_Int : TypeDescriptor
{
    TypeDescriptor_Int() : TypeDescriptor{"int", sizeof(int), std::type_index(typeid(int))} {}
    virtual void Dump(const void* obj, int) const override
    {
        std::cout << "int{" << *(const int*) obj << "}";
    }
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
    TypeDescriptor_Float() : TypeDescriptor{"float", sizeof(float), std::type_index(typeid(float))} {}
    virtual void Dump(const void* obj, int) const override
    {
        std::cout << "int{" << *(const float*) obj << "}";
    }
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
    TypeDescriptor_Bool() : TypeDescriptor{"Bool", sizeof(bool), std::type_index(typeid(bool))} {}
    virtual void Dump(const void* obj, int) const override
    {
        std::cout << "bool{" << *(const bool*) obj << "}";
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<bool>()
{
    static TypeDescriptor_Bool typeDesc;
    return &typeDesc;
}
} // namespace aln::reflect
