/// Descriptors for types from the GLM library.
#include "reflection.hpp"

#include <glm/vec3.hpp>

// TODO: For now we're limited by namespace'd types.
// Find a good way to allow using the macro for them as well
namespace aln::reflect
{

/// @brief A type descriptor for glm::vec3
struct TypeDescriptor_GlmVec3 : TypeDescriptor
{
    TypeDescriptor_GlmVec3() : TypeDescriptor(aln::StringID("glm::vec3")) {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<glm::vec3>()
{
    static TypeDescriptor_GlmVec3 typeDesc;
    return &typeDesc;
}
} // namespace aln::reflect