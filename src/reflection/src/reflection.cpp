#include "reflection.hpp"
#include <map>

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

void TypeDescriptor_Struct::Dump(const void* obj, int indentLevel) const
{
    std::cout << name << " {" << std::endl;
    for (const Member& member : members)
    {
        std::cout << std::string(4 * (indentLevel + 1), ' ') << member.name << " = ";
        member.type->Dump((char*) obj + member.offset, indentLevel + 1);
        std::cout << std::endl;
    }
    std::cout << std::string(4 * indentLevel, ' ') << "}";
}

void TypeDescriptor_Struct::InEditor(void* obj, const char* fieldName) const
{
    if (ImGui::CollapsingHeader(fieldName))
    {
        for (const Member& member : members)
        {
            member.type->InEditor((char*) obj + member.offset, member.name);
        }
    }
}

std::vector<TypeDescriptor*>& GetTypesInScope(std::string scopeName)
{
    static std::map<std::string, std::vector<TypeDescriptor*>> typeScopes;
    auto it = typeScopes.try_emplace(scopeName);
    return it.first->second;
}

} // namespace aln::reflect