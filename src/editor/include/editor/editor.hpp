#pragma once

#include <concepts>

#include <imgui.h>

#include <reflection/reflection.hpp>

#include <entities/component.hpp>

#include <functional>
#include <typeindex>

namespace aln::editor
{
// ImGui methods to set the context and allocator functions in case reflection is in a separate library.
void SetImGuiContext(ImGuiContext* pContext);
void SetImGuiAllocatorFunctions(ImGuiMemAllocFunc* pAllocFunc, ImGuiMemFreeFunc* pFreeFunc, void** pUserData);

class Editor
{
  private:
    std::unordered_map<std::type_index, std::function<void(void*, const char*)>> m_displayFuncs;

    template <typename T>
    void RegisterType();

    template <typename T>
    void Display(void* obj, const char* label);
    void Display(std::type_index typeIndex, void* obj, const char* label);

    void DisplayTypeStruct(const reflect::TypeDescriptor_Struct* pType, void* obj);

  public:
    Editor();

    template <typename T>
    void InInspector(T* obj, const char* label)
    {
        Display<T>(obj, label);
    }
};
} // namespace aln::editor