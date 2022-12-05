#include "types_editor.hpp"
#include <imgui_stdlib.h>

#include <assets/asset_id.hpp>

namespace aln
{

TypeEditorService::TypeEditorService()
{
    /// This is not ideal, but is required to be able to display the members of reflected structs
    /// (which do not know their actual type)
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
    RegisterType<AssetID>();
}

template <typename T>
void TypeEditorService::Display(T* ptr, const char* label)
{
    m_displayFuncs.at(std::type_index(typeid(T)))(ptr, label);
}

template <>
void TypeEditorService::Display<IComponent>(IComponent* pComponent, const char* label)
{
    DisplayTypeStruct(pComponent->GetType(), pComponent);
}

template <>
void TypeEditorService::Display<IEntitySystem>(IEntitySystem* pSystem, const char* label)
{
    DisplayTypeStruct(pSystem->GetType(), pSystem);
}

template <>
void TypeEditorService::Display<int>(int* i, const char* label)
{
    ImGui::InputInt(label, i);
}

template <>
void TypeEditorService::Display<float>(float* i, const char* label)
{
    ImGui::InputFloat(label, i);
}

template <>
void TypeEditorService::Display<bool>(bool* b, const char* label)
{
    ImGui::Checkbox(label, b);
}

template <>
void TypeEditorService::Display<AssetHandle<Mesh>>(AssetHandle<Mesh>* pHandle, const char* label)
{
    auto pMesh = pHandle->get();
    if (ImGui::CollapsingHeader("Mesh"))
    {
        // TODO: Edition
        ImGui::Text(pMesh->GetID().GetAssetPath().c_str());
    }
}

template <>
void TypeEditorService::Display<AssetHandle<AnimationClip>>(AssetHandle<AnimationClip>* pHandle, const char* label)
{
    auto pAnim = pHandle->get();
    if (ImGui::CollapsingHeader("AnimationClip"))
    {
        // TODO: Edition
        ImGui::Text(pAnim->GetID().GetAssetPath().c_str());
    }
}

template <>
void TypeEditorService::Display<AssetHandle<Material>>(AssetHandle<Material>* pHandle, const char* label)
{
    auto pMaterial = pHandle->get();

    if (ImGui::CollapsingHeader("Material"))
    {
        // TODO: Edition
        ImGui::Text(pMaterial->GetID().GetAssetPath().c_str());
    }
}

template <>
void TypeEditorService::Display<Texture>(Texture* pTexture, const char* label)
{
    ImGui::Text(pTexture->GetID().GetAssetPath().c_str());
}

template <>
void TypeEditorService::Display<RGBAColor>(RGBAColor* pColor, const char* label)
{
    ImGui::ColorEdit4("##picker", (float*) pColor);
    ImGui::SameLine();
    ImGui::Text(label);
}

template <>
void TypeEditorService::Display<RGBColor>(RGBColor* pColor, const char* label)
{
    ImGui::ColorEdit3("##picker", (float*) pColor);
    ImGui::SameLine();
    ImGui::Text(label);
}

template <>
void TypeEditorService::Display<glm::vec3>(glm::vec3* pVec, const char* label)
{
    ImGui::DragFloat((std::string("x##") + label).c_str(), &pVec->x, 1.0f);
    ImGui::SameLine();
    ImGui::DragFloat((std::string("y##") + label).c_str(), &pVec->y, 1.0f);
    ImGui::SameLine();
    ImGui::DragFloat((std::string("z##") + label).c_str(), &pVec->z, 1.0f);
}

template <>
void TypeEditorService::Display<std::string>(std::string* pString, const char* label)
{
    ImGui::InputText(label, pString, pString->size());
}

template <>
void TypeEditorService::Display<AssetID>(AssetID* pID, const char* label)
{
    ImGui::Text(pID->GetAssetPath().c_str());
    // TODO
}
} // namespace aln