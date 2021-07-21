#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "reflection.hpp"

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
} // namespace aln::reflect