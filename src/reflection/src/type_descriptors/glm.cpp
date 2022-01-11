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
    TypeDescriptor_GlmVec3() : TypeDescriptor{"glm::vec3", sizeof(glm::vec3), std::type_index(typeid(glm::vec3))} {}
    virtual void Dump(const void* obj, int) const override
    {
        glm::vec3* vec = (glm::vec3*) obj;

        std::cout << "glm::vec3{" << glm::to_string(*vec) << "}";
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<glm::vec3>()
{
    static TypeDescriptor_GlmVec3 typeDesc;
    return &typeDesc;
}
} // namespace aln::reflect