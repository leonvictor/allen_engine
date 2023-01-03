/// Descriptors for types from the GLM library.
#include "reflection.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace aln::reflect
{

/// @brief A type descriptor for glm::vec3
struct TypeDescriptor_GlmVec3 : TypeDescriptor
{
    TypeDescriptor_GlmVec3() : TypeDescriptor{std::type_index(typeid(glm::vec3))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<glm::vec3>()
{
    static TypeDescriptor_GlmVec3 typeDesc;
    return &typeDesc;
}
} // namespace aln::reflect