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
    TypeDescriptor_AlnRGBAColor() : TypeDescriptor{"aln::RGBAColor", sizeof(aln::RGBAColor), std::type_index(typeid(aln::RGBAColor))} {}
    virtual void Dump(const void* obj, int) const override
    {
        glm::vec4* color = (glm::vec4*) obj;
        std::cout << "aln::RGBAColor{" << glm::to_string(*color) << "}";
    }
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
    TypeDescriptor_AlnRGBColor() : TypeDescriptor{"aln::RGBColor", sizeof(aln::RGBColor), std::type_index(typeid(aln::RGBColor))} {}
    virtual void Dump(const void* obj, int) const override
    {
        glm::vec4* color = (glm::vec4*) obj;
        std::cout << "aln::RGBColor{" << glm::to_string(*color) << "}";
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<aln::RGBColor>()
{
    static TypeDescriptor_AlnRGBColor typeDesc;
    return &typeDesc;
}
} // namespace aln::reflect
