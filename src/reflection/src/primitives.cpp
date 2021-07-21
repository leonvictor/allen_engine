#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "reflection.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace aln::reflect
{

//--------------------------------------------------------
// A type descriptor for int
//--------------------------------------------------------

struct TypeDescriptor_Int : TypeDescriptor
{
    TypeDescriptor_Int() : TypeDescriptor{"int", sizeof(int)}
    {
    }
    virtual void Dump(const void* obj, int) const override
    {
        std::cout << "int{" << *(const int*) obj << "}";
    }

    virtual void InEditor(void* obj, const char* fieldName = "") const override
    {
        ImGui::InputInt(fieldName, (int*) obj);
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<int>()
{
    static TypeDescriptor_Int typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// A type descriptor for std::string
//--------------------------------------------------------

struct TypeDescriptor_StdString : TypeDescriptor
{
    TypeDescriptor_StdString() : TypeDescriptor{"std::string", sizeof(std::string)}
    {
    }
    virtual void Dump(const void* obj, int) const override
    {
        std::cout << "std::string{\"" << *(const std::string*) obj << "\"}";
    }

    virtual void InEditor(void* obj, const char* fieldName = "") const override
    {
        auto str = (std::string*) obj;
        ImGui::InputText(fieldName, str, str->size());
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<std::string>()
{
    static TypeDescriptor_StdString typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// A type descriptor for float
//--------------------------------------------------------

struct TypeDescriptor_Float : TypeDescriptor
{
    TypeDescriptor_Float() : TypeDescriptor{"float", sizeof(float)}
    {
    }
    virtual void Dump(const void* obj, int) const override
    {
        std::cout << "int{" << *(const float*) obj << "}";
    }

    virtual void InEditor(void* obj, const char* fieldName = "") const override
    {
        ImGui::InputFloat(fieldName, (float*) obj);
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<float>()
{
    static TypeDescriptor_Float typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// A type descriptor for bool
//--------------------------------------------------------

struct TypeDescriptor_Bool : TypeDescriptor
{
    TypeDescriptor_Bool() : TypeDescriptor{"Bool", sizeof(bool)}
    {
    }
    virtual void Dump(const void* obj, int) const override
    {
        std::cout << "bool{" << *(const bool*) obj << "}";
    }

    virtual void InEditor(void* obj, const char* fieldName = "") const override
    {
        ImGui::Checkbox(fieldName, (bool*) obj);
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<bool>()
{
    static TypeDescriptor_Bool typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// A type descriptor for std::string
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
} // namespace aln::reflect
