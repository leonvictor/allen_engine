/// Descriptors for custom color types.

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "reflection.hpp"

#include <common/colors.hpp>

#include <glm/gtx/string_cast.hpp>
#include <string>

namespace aln::reflect
{

//--------------------------------------------------------
// A type descriptor for RGBAColor
//--------------------------------------------------------
struct TypeDescriptor_AlnRGBAColor : TypeDescriptor
{
    TypeDescriptor_AlnRGBAColor() : TypeDescriptor{"aln::RGBAColor", sizeof(aln::RGBAColor)}
    {
    }

    virtual void Dump(const void* obj, int) const override
    {
        glm::vec4* color = (glm::vec4*) obj;

        std::cout << "aln::RGBAColor{" << glm::to_string(*color) << "}";
    }

    virtual void InEditor(void* obj, const char* fieldName = "") const override
    {
        aln::RGBAColor* color = (aln::RGBAColor*) obj;
        ImGui::ColorEdit4("##picker", (float*) color);
        ImGui::SameLine();
        ImGui::Text(fieldName);
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<aln::RGBAColor>()
{
    static TypeDescriptor_AlnRGBAColor typeDesc;
    return &typeDesc;
}
}