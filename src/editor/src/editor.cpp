#include "editor.hpp"
#include "animation_graph/animation_graph_editor.hpp"

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

void SetImGuiContext(const EditorImGuiContext& context)
{
    ImGui::SetAllocatorFunctions(*context.m_pAllocFunc, *context.m_pFreeFunc, context.m_pUserData);
    ImGui::SetCurrentContext(context.m_pImGuiContext);

    ImNodes::SetCurrentContext(context.m_pImNodesContext);
    ImNodes::SetImGuiContext(context.m_pImGuiContext);
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
void Editor::Display<IComponent>(void* ptr, const char* label)
{
    auto pComp = (IComponent*) ptr;
    auto pType = pComp->GetType();
    DisplayTypeStruct(pType, pComp);
}

template <>
void Editor::Display<IEntitySystem>(void* ptr, const char* label)
{
    auto pSys = (IEntitySystem*) ptr;
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
        ImGui::Text(pMesh->GetID().GetAssetPath().c_str());
    }
}

template <>
void Editor::Display<AssetHandle<AnimationClip>>(void* pHandle, const char* label)
{
    auto pAnim = ((AssetHandle<AnimationClip>*) pHandle)->get();
    if (ImGui::CollapsingHeader("AnimationClip"))
    {
        // TODO: Edition
        ImGui::Text(pAnim->GetID().GetAssetPath().c_str());
    }
}

template <>
void Editor::Display<AssetHandle<Material>>(void* pHandle, const char* label)
{
    auto pMaterial = ((AssetHandle<Material>*) pHandle)->get();

    if (ImGui::CollapsingHeader("Material"))
    {
        // TODO: Edition
        ImGui::Text(pMaterial->GetID().GetAssetPath().c_str());
    }
}

template <>
void Editor::Display<Texture>(void* pTexture, const char* label)
{
    ImGui::Text(((Texture*) pTexture)->GetID().GetAssetPath().c_str());
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
Editor::Editor(WorldEntity& worldEntity) : m_worldEntity(worldEntity)
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

void Editor::DrawUI(const vk::DescriptorSet& renderedSceneImageDescriptorSet, float deltaTime)
{
    // Draw ImGUI components
    ImGuiViewportP* viewport = (ImGuiViewportP*) (void*) ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();

    auto dockID = ImGui::DockSpaceOverViewport(viewport);

    // TODO: Programatically set the initial layout
    if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("Item");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Item");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug"))
            {
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();

    if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height, window_flags))
    {
        if (ImGui::BeginMenuBar())
        {
            // Compute current FPS
            // Use std::format (C++20). Not available in most compilers as of 04/06/2021
            ImGui::Text(fmt::format("{:.0f} FPS", 1.0f / deltaTime).c_str());
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();

    if (ImGui::Begin(ICON_FA_GLOBE " Scene", nullptr, ImGuiWindowFlags_NoScrollbar))
    {
        // Update current scene preview dims
        // @todo: use a dedicated struct for dimensions
        auto dim = ImGui::GetContentRegionAvail();
        m_scenePreviewWidth = dim.x;
        m_scenePreviewHeight = dim.y;
        ImGui::Image((ImTextureID) renderedSceneImageDescriptorSet, {m_scenePreviewWidth, m_scenePreviewHeight});
    }
    ImGui::End();

    if (ImGui::Begin("Animation Graph", nullptr))
    {
        // TMP: Create a graph editor
        if (m_pAnimationGraphEditor == nullptr)
        {
            m_pAnimationGraphEditor = aln::New<AnimationGraphEditor>();
        }

        m_pAnimationGraphEditor->Draw();
    }
    else
    {
        if (m_pAnimationGraphEditor != nullptr)
        {
            // TODO: Serialize
            aln::Delete(m_pAnimationGraphEditor);
        }
    }
    ImGui::End();

    if (ImGui::Begin("LogsViewport", nullptr, ImGuiWindowFlags_NoTitleBar))
    {
        if (ImGui::BeginTabBar("LogsTabBar"))
        {
            if (ImGui::BeginTabItem("Logs"))
            {
                ImGui::Text("Sample Logs");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    // Inspector panel
    if (m_pSelectedEntity != nullptr)
    {
        if (ImGui::Begin(ICON_FA_INFO_CIRCLE " Inspector", nullptr))
        {
            ImGui::PushID(m_pSelectedEntity->GetID().ToString().c_str());

            ImGui::AlignTextToFramePadding();
            ImGui::Text(ICON_FA_CUBES);
            ImGui::SameLine();
            ImGui::InputText("", &m_pSelectedEntity->GetName());
            ImGui::SameLine(ImGui::GetWindowWidth() - 30);

            if (m_pSelectedEntity->IsActivated())
            {
                ImGui::Text(ICON_FA_EYE);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Disable entity");
                }
                if (ImGui::IsItemClicked())
                {
                    m_worldEntity.DeactivateEntity(m_pSelectedEntity);
                }
            }
            else
            {
                ImGui::Text(ICON_FA_EYE_SLASH);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Enable entity");
                }
                if (ImGui::IsItemClicked())
                {
                    m_worldEntity.ActivateEntity(m_pSelectedEntity);
                }
            }

            if (m_pSelectedEntity->IsSpatialEntity())
            {
                if (ImGui::CollapsingHeader("Transform"))
                {
                    ImGui::PushItemWidth(60);

                    // TODO: vec3 might deserve a helper function to create ui for the 3 components...
                    // Position
                    bool changed = false;
                    auto pos = m_pSelectedEntity->GetRootSpatialComponent()->GetLocalTransform().GetTranslation();
                    ImGui::Text("Position");
                    changed |= ImGui::DragFloat("x##Position", &pos.x, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("y##Position", &pos.y, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("z##Position", &pos.z, 1.0f);

                    if (changed)
                        m_pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformPosition(pos);

                    // Rotation
                    changed = false;
                    ImGui::Text("Rotation");
                    changed |= ImGui::DragFloat("x##Rotation", &m_currentEulerRotation.x, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("y##Rotation", &m_currentEulerRotation.y, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("z##Rotation", &m_currentEulerRotation.z, 1.0f);

                    if (changed)
                        m_pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformRotationEuler(m_currentEulerRotation);

                    // Scale
                    auto scale = m_pSelectedEntity->GetRootSpatialComponent()->GetLocalTransform().GetScale();
                    changed = false;
                    ImGui::Text("Scale");
                    changed |= ImGui::DragFloat("x##Scale", &scale.x, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("y##Scale", &scale.y, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("z##Scale", &scale.z, 1.0f);

                    if (changed)
                        m_pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformScale(scale);

                    ImGui::PopItemWidth();
                }
            }

            for (auto pComponent : m_pSelectedEntity->GetComponents())
            {
                ImGui::PushID(pComponent->GetID().ToString().c_str());

                auto typeDesc = pComponent->GetType();
                // TODO: This should happen through the partial template specialization for components

                if (ImGui::CollapsingHeader(typeDesc->GetPrettyName().c_str(), ImGuiTreeNodeFlags_AllowItemOverlap))
                {
                    InInspector(pComponent, "");
                }

                if (ImGui::BeginPopupContextItem("context_popup", ImGuiPopupFlags_MouseButtonRight))
                {
                    if (ImGui::MenuItem("Remove Component", "", false, true))
                    {
                        m_pSelectedEntity->DestroyComponent(pComponent->GetID());
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            for (auto pSystem : m_pSelectedEntity->GetSystems())
            {
                auto typeDesc = pSystem->GetType();

                ImGui::PushID(typeDesc->GetPrettyName().c_str());
                if (ImGui::CollapsingHeader(typeDesc->GetPrettyName().c_str()))
                {
                    InInspector(pSystem, "");
                }

                if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
                {
                    if (ImGui::MenuItem("Remove System", "", false, true))
                    {
                        m_pSelectedEntity->DestroySystem(pSystem->GetType());
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            if (ImGui::Button("Add Component"))
            {
                // Open a dropdown with all components types
                ImGui::OpenPopup("add_component_popup");
            }

            ImGui::SameLine();
            if (ImGui::Button("Add System"))
            {
                ImGui::OpenPopup("add_system_popup");
            }

            if (ImGui::BeginPopup("add_system_popup"))
            {
                auto& systemTypes = aln::reflect::GetTypesInScope("SYSTEMS");
                for (auto& sys : systemTypes)
                {
                    if (ImGui::Selectable(sys->GetPrettyName().c_str()))
                    {
                        m_pSelectedEntity->CreateSystem(sys);
                    }
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("add_component_popup"))
            {
                auto& componentTypes = aln::reflect::GetTypesInScope("COMPONENTS");
                for (auto& comp : componentTypes)
                {
                    if (ImGui::Selectable(comp->GetPrettyName().c_str()))
                    {
                        auto newComp = comp->typeHelper->CreateType<IComponent>();
                        m_pSelectedEntity->AddComponent(newComp);
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
        ImGui::End();
    }

    // Outline panel
    if (ImGui::Begin(ICON_FA_LIST " Outline"))
    {
        auto& entities = m_worldEntity.GetEntities();
        for (auto pEntity : entities)
        {
            if (!pEntity->HasParentEntity())
            {
                RecurseEntityTree(pEntity);
            }
        }

        // Add a dummy panel to the rest of the outline pane
        ImGui::Dummy(ImGui::GetWindowSize());

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                assert(payload->DataSize == sizeof(Entity**));
                Entity* entityPayload = *((Entity**) payload->Data);
                entityPayload->SetParentEntity(nullptr);
            }
            ImGui::EndDragDropTarget();
        }

        EntityOutlinePopup(nullptr);
    }
    ImGui::End();

    ImGui::ShowDemoWindow();
}

void Editor::RecurseEntityTree(Entity* pEntity)
{
    ImGui::PushID(pEntity->GetID().ToString().c_str());
    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
    ImGuiTreeNodeFlags node_flags = base_flags;

    if (m_pSelectedEntity != nullptr && m_pSelectedEntity == pEntity)
    {
        node_flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool hasChildren = pEntity->HasChildrenEntities();
    if (!hasChildren)
    {
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (!pEntity->IsActivated())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    }

    // We add the id to the ImGui hash to differentiate entities with the same name
    bool node_open = ImGui::TreeNodeEx(pEntity->GetName().c_str(), node_flags);

    EntityOutlinePopup(pEntity);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        m_pSelectedEntity = pEntity;
        if (pEntity->IsSpatialEntity())
        {
            m_currentEulerRotation = pEntity->GetRootSpatialComponent()->GetLocalTransform().GetRotationEuler();
        }
    }

    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("ENTITY", &pEntity, sizeof(Entity**));
        ImGui::Text(pEntity->GetName().c_str());
        ImGui::EndDragDropSource();
    }

    // Entity drag and drop target
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            assert(payload->DataSize == sizeof(Entity**));

            Entity* entityPayload = *((Entity**) payload->Data);

            if (entityPayload->IsSpatialEntity())
            {
                entityPayload->SetParentEntity(pEntity);

                // Set the receiving node as open
                ImGui::GetStateStorage()->SetInt(ImGui::GetID(pEntity->GetName().c_str()), 1);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (node_open && hasChildren)
    {
        ImGui::Indent();
        for (auto child : pEntity->GetChildren())
        {
            RecurseEntityTree(child);
        }
        ImGui::Unindent();
    }

    if (!pEntity->IsActivated())
    {
        ImGui::PopStyleColor();
    }

    ImGui::PopID();
}

void Editor::EntityOutlinePopup(Entity* pEntity)
{
    if (ImGui::BeginPopupContextItem("entity_outline_popup", ImGuiPopupFlags_MouseButtonRight))
    {
        auto contextEntityAndNotSpatial = (pEntity != nullptr && !pEntity->IsSpatialEntity());

        if (pEntity != nullptr)
        {
            ImGui::Text(pEntity->GetID().ToString().c_str());
            if (ImGui::MenuItem("Remove Entity"))
            {
                if (m_pSelectedEntity == pEntity)
                {
                    m_pSelectedEntity = nullptr;
                }
                m_worldEntity.m_entityMap.RemoveEntity(pEntity);
            }
        }
        if (ImGui::MenuItem("Add Empty Entity", "", false, pEntity == nullptr))
        {
            auto* pNewEntity = m_worldEntity.m_entityMap.CreateEntity("Entity");
            // TODO: This will be useful with other options, but empty entities are not spatial so they can't be attached to others.
            // if (pEntity != nullptr)
            // {
            //     pNewEntity->SetParentEntity(pEntity);
            // }
        }
        ImGui::EndPopup();
    }
}
} // namespace aln::editor