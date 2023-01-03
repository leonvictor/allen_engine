/// Descriptors for custom color types.
#include "reflection.hpp"

#include <common/colors.hpp>

#include <glm/gtx/string_cast.hpp>
#include <string>

namespace aln::reflect
{

/// @brief A type descriptor for RGBAColor
struct TypeDescriptor_AlnRGBAColor : TypeDescriptor
{
    TypeDescriptor_AlnRGBAColor() : TypeDescriptor{std::type_index(typeid(aln::RGBAColor))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<aln::RGBAColor>()
{
    static TypeDescriptor_AlnRGBAColor typeDesc;
    return &typeDesc;
}

/// @brief A type descriptor for RGBAColor
struct TypeDescriptor_AlnRGBColor : TypeDescriptor
{
    TypeDescriptor_AlnRGBColor() : TypeDescriptor(std::type_index(typeid(aln::RGBColor))) {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<aln::RGBColor>()
{
    static TypeDescriptor_AlnRGBColor typeDesc;
    return &typeDesc;
}
} // namespace aln::reflect
