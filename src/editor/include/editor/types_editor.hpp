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

    /// @brief Wrapper around display function accepting void pointers, used in the display functions map
    template <typename T>
    void RegisteredDisplay(void* ptr, const char* label) const
    {
        return Display<T>((T*) ptr, label);
    }

    template <typename T>
    void RegisterType()
    {
        m_displayFuncs[std::type_index(typeid(T))] = std::bind(&TypeEditorService::RegisteredDisplay<T>, this, std::placeholders::_1, std::placeholders::_2);
    }

    /// @brief Display a type by looking up its typeIndex in the map. Used only for reflected structs' members for which the runtime
    // type is not available.
    void Display(std::type_index typeIndex, void* obj, const char* label) const
    {
        m_displayFuncs.at(typeIndex)(obj, label);
    }

  public:
    TypeEditorService();

    template <typename T>
    void Display(T* ptr, const char* label) const;

    void DisplayTypeStruct(const reflect::TypeDescriptor_Struct* pType, void* obj) const
    {
        for (auto& member : pType->m_members)
        {
            Display(member.type->m_typeIndex, (char*) obj + member.offset, member.displayName);
        }
    }
};
} // namespace aln
