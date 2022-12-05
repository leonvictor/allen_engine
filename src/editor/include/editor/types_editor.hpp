#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>

#include <reflection/reflection.hpp>

#include <anim/animation_clip.hpp>
#include <assets/handle.hpp>
#include <common/colors.hpp>
#include <core/material.hpp>
#include <core/mesh.hpp>
#include <core/texture.hpp>
#include <entities/component.hpp>
#include <entities/entity_system.hpp>

namespace aln
{
/// @brief Service class used to display reflected types in the editor
class TypeEditorService
{
  private:
    std::unordered_map<std::type_index, std::function<void(void*, const char*)>> m_displayFuncs;

    template <typename T>
    void RegisteredDisplay(void* ptr, const char* label)
    {
        return Display<T>((T*) ptr, label);
    }

  public:
    TypeEditorService();

    template <typename T>
    void RegisterType()
    {
        m_displayFuncs[std::type_index(typeid(T))] = std::bind(&TypeEditorService::RegisteredDisplay<T>, this, std::placeholders::_1, std::placeholders::_2);
    }

    template <typename T>
    void Display(T* ptr, const char* label);

    void Display(std::type_index typeIndex, void* obj, const char* label)
    {
        m_displayFuncs.at(typeIndex)(obj, label);
    }

    // template <typename T>
    // void Display(void* ptr, const char* label);

    void DisplayTypeStruct(const reflect::TypeDescriptor_Struct* pType, void* obj)
    {
        ImGui::Indent();
        {
            for (auto& member : pType->members)
            {
                Display(member.type->m_typeIndex, (char*) obj + member.offset, member.GetPrettyName().c_str());
            }
        }
        ImGui::Unindent();
    }
};
} // namespace aln
