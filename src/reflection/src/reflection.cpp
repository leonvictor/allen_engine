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
} // namespace aln::reflect