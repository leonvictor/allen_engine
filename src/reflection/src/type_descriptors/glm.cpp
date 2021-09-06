/// Descriptors for types from the GLM library.

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "reflection.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace aln::reflect
{

//--------------------------------------------------------
// A type descriptor for glm::vec3
//--------------------------------------------------------
struct TypeDescriptor_GlmVec3 : TypeDescriptor
{
    TypeDescriptor_GlmVec3() : TypeDescriptor{"glm::vec3", sizeof(glm::vec3)}
    {
    }

    virtual void Dump(const void* obj, int) const override
    {
        glm::vec3* vec = (glm::vec3*) obj;

        std::cout << "glm::vec3{" << glm::to_string(*vec) << "}";
    }

    virtual void InEditor(void* obj, const char* fieldName = "") const override
    {
        ImGui::Text(fieldName);
        glm::vec3* vec = (glm::vec3*) obj;
        ImGui::DragFloat((std::string("x##") + fieldName).c_str(), &vec->x, 1.0f);
        ImGui::SameLine();
        ImGui::DragFloat((std::string("y##") + fieldName).c_str(), &vec->y, 1.0f);
        ImGui::SameLine();
        ImGui::DragFloat((std::string("z##") + fieldName).c_str(), &vec->z, 1.0f);
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<glm::vec3>()
{
    static TypeDescriptor_GlmVec3 typeDesc;
    return &typeDesc;
}
}