#include "editor.hpp"

#include "misc/cpp/imgui_stdlib.h"

#include <assets/handle.hpp>
#include <common/colors.hpp>
#include <core/material.hpp>
#include <core/mesh.hpp>
#include <core/texture.hpp>

#include <anim/animation_clip.hpp>

#include <entities/entity_system.hpp>

#include <glm/vec3.hpp>

namespace aln::editor
{
void SetImGuiContext(ImGuiContext* pContext)
{
    ImGui::SetCurrentContext(pContext);
}

void SetImGuiAllocatorFunctions(ImGuiMemAllocFunc* pAllocFunc, ImGuiMemFreeFunc* pFreeFunc, void** pUserData)
{
    ImGui::SetAllocatorFunctions(*pAllocFunc, *pFreeFunc, *pUserData);
}

template <typename T>
void Editor::Display(void* obj, const char* label)
{
    std::string n = std::type_index(typeid(T)).name();
    m_displayFuncs.at(std::type_index(typeid(T)))(obj, label);
}

void Editor::Display(std::type_index typeIndex, void* obj, const char* label)
{
    m_displayFuncs.at(typeIndex)(obj, label);
}

void Editor::DisplayTypeStruct(const reflect::TypeDescriptor_Struct* pType, void* obj)
{
    ImGui::Indent();
    {
        for (auto& member : pType->members)
        {
            Editor::Display(member.type->m_typeIndex, (char*) obj + member.offset, member.GetPrettyName().c_str());
        }
    }
    ImGui::Unindent();
}

template <>
void Editor::Display<entities::IComponent>(void* ptr, const char* label)
{
    auto pComp = (entities::IComponent*) ptr;
    auto pType = pComp->GetType();
    DisplayTypeStruct(pType, pComp);
}

template <>
void Editor::Display<entities::IEntitySystem>(void* ptr, const char* label)
{
    auto pSys = (entities::IEntitySystem*) ptr;
    auto pType = pSys->GetType();
    DisplayTypeStruct(pType, pSys);
}

template <>
void Editor::Display<int>(void* i, const char* label)
{
    ImGui::InputInt(label, (int*) i);
}

template <>
void Editor::Display<float>(void* i, const char* label)
{
    ImGui::InputFloat(label, (float*) i);
}

template <>
void Editor::Display<bool>(void* b, const char* label)
{
    ImGui::Checkbox(label, (bool*) b);
}

template <typename T>
void Editor::RegisterType()
{
    m_displayFuncs[std::type_index(typeid(T))] = std::bind(&Editor::Display<T>, this, std::placeholders::_1, std::placeholders::_2);
}

template <>
void Editor::Display<AssetHandle<Mesh>>(void* pHandle, const char* label)
{
    auto pMesh = ((AssetHandle<Mesh>*) pHandle)->get();
    if (ImGui::CollapsingHeader("Mesh"))
    {
        // TODO: Edition
        ImGui::Text(pMesh->GetID().c_str());
    }
}

template <>
void Editor::Display<AssetHandle<AnimationClip>>(void* pHandle, const char* label)
{
    auto pAnim = ((AssetHandle<AnimationClip>*) pHandle)->get();
    if (ImGui::CollapsingHeader("AnimationClip"))
    {
        // TODO: Edition
        ImGui::Text(pAnim->GetID().c_str());
    }
}

template <>
void Editor::Display<AssetHandle<Material>>(void* pHandle, const char* label)
{
    auto pMaterial = ((AssetHandle<Material>*) pHandle)->get();

    if (ImGui::CollapsingHeader("Material"))
    {
        // TODO: Edition
        ImGui::Text(pMaterial->GetID().c_str());
    }
}

template <>
void Editor::Display<Texture>(void* pTexture, const char* label)
{
    ImGui::Text(((Texture*) pTexture)->GetID().c_str());
}

template <>
void Editor::Display<aln::RGBAColor>(void* pColor, const char* label)
{
    ImGui::ColorEdit4("##picker", (float*) pColor);
    ImGui::SameLine();
    ImGui::Text(label);
}

template <>
void Editor::Display<aln::RGBColor>(void* pColor, const char* label)
{
    ImGui::ColorEdit3("##picker", (float*) pColor);
    ImGui::SameLine();
    ImGui::Text(label);
}

template <>
void Editor::Display<glm::vec3>(void* ptr, const char* label)
{
    glm::vec3* pVec = (glm::vec3*) ptr;
    ImGui::DragFloat((std::string("x##") + label).c_str(), &pVec->x, 1.0f);
    ImGui::SameLine();
    ImGui::DragFloat((std::string("y##") + label).c_str(), &pVec->y, 1.0f);
    ImGui::SameLine();
    ImGui::DragFloat((std::string("z##") + label).c_str(), &pVec->z, 1.0f);
}

template <>
void Editor::Display<std::string>(void* ptr, const char* label)
{
    auto pString = (std::string*) ptr;
    ImGui::InputText(label, pString, pString->size());
}
// Poopy
Editor::Editor()
{
    RegisterType<int>();
    RegisterType<float>();
    RegisterType<bool>();
    RegisterType<AssetHandle<Mesh>>();
    RegisterType<AssetHandle<Material>>();
    RegisterType<AssetHandle<AnimationClip>>();
    RegisterType<RGBColor>();
    RegisterType<RGBAColor>();
    RegisterType<glm::vec3>();
    RegisterType<std::string>();
}

} // namespace aln::editor