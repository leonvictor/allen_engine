#include "reflection.hpp"

namespace aln::reflect
{
void SetImGuiContext(ImGuiContext* pContext)
{
    ImGui::SetCurrentContext(pContext);
}
void SetImGuiAllocatorFunctions(ImGuiMemAllocFunc* pAllocFunc, ImGuiMemFreeFunc* pFreeFunc, void** pUserData)
{
    ImGui::SetAllocatorFunctions(*pAllocFunc, *pFreeFunc, *pUserData);
}

void TypeDescriptor_Struct::InEditor(void* obj, const char* fieldName) const
{
    if (ImGui::CollapsingHeader(name))
    {
        for (const Member& member : members)
        {
            member.type->InEditor((char*) obj + member.offset, member.name);
        }
    }
}
} // namespace aln::reflect