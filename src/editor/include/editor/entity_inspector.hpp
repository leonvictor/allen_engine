#pragma once

#include "editor_window.hpp"
#include "reflected_types/reflected_type_editor.hpp"

namespace aln
{
class EntityInspector : public IEditorWindow
{
    ReflectedTypeEditor m_entityComponentsSystemInspector;
    UUID m_entityStartedEditingEventID;
    UUID m_entityCompletedEditingEventID;

    // TODO: Move to the quaternion widget directly
    glm::vec3 m_currentEulerRotation; // Inspector's rotation is stored separately to avoid going back and forth between quat and euler
    Entity* m_pEntity;

    const TypeRegistryService* m_pTypeRegistryService = nullptr;

  public:
    void Initialize(EditorWindowContext* pEditorWindowContext)
    {
        IEditorWindow::Initialize(pEditorWindowContext);
        m_pTypeRegistryService = pEditorWindowContext->m_pTypeRegistryService;
    }

    virtual void Update(const UpdateContext& context)
    {
        auto pWorldEntity = GetWorldEntity();
        auto pSelectedEntity = GetSelectedEntity();
        if (pSelectedEntity != m_pEntity)
        {
            m_pEntity = pSelectedEntity;
            if (m_pEntity != nullptr && m_pEntity->IsSpatialEntity())
            {
                m_currentEulerRotation = m_pEntity->GetRootSpatialComponent()->GetLocalTransform().GetRotationEuler();
            }
        }

        // Inspector panel
        if (ImGui::Begin(ICON_FA_INFO_CIRCLE " Inspector", nullptr) && pSelectedEntity != nullptr)
        {
            ImGui::PushID(pSelectedEntity->GetID().ToString().c_str());

            ImGui::AlignTextToFramePadding();
            ImGui::Text(ICON_FA_CUBES);
            ImGui::SameLine();
            ImGui::InputText("", &pSelectedEntity->GetName());
            ImGui::SameLine(ImGui::GetWindowWidth() - 30);

            if (pSelectedEntity->IsActivated())
            {
                ImGui::Text(ICON_FA_EYE);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Disable entity");
                }
                if (ImGui::IsItemClicked())
                {
                    pWorldEntity->DeactivateEntity(pSelectedEntity);
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
                    pWorldEntity->ActivateEntity(pSelectedEntity);
                }
            }

            if (pSelectedEntity->IsSpatialEntity())
            {
                if (ImGui::CollapsingHeader("Transform"))
                {
                    ImGui::PushItemWidth(60);

                    // TODO: vec3 might deserve a helper function to create ui for the 3 components...
                    // Position
                    bool changed = false;
                    auto pos = pSelectedEntity->GetRootSpatialComponent()->GetLocalTransform().GetTranslation();
                    ImGui::Text("Position");
                    changed |= ImGui::DragFloat("x##Position", &pos.x, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("y##Position", &pos.y, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("z##Position", &pos.z, 1.0f);

                    if (changed)
                        pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformPosition(pos);

                    // Rotation
                    changed = false;
                    ImGui::Text("Rotation");
                    changed |= ImGui::DragFloat("x##Rotation", &m_currentEulerRotation.x, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("y##Rotation", &m_currentEulerRotation.y, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("z##Rotation", &m_currentEulerRotation.z, 1.0f);

                    if (changed)
                        pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformRotationEuler(m_currentEulerRotation);

                    // Scale
                    auto scale = pSelectedEntity->GetRootSpatialComponent()->GetLocalTransform().GetScale();
                    changed = false;
                    ImGui::Text("Scale");
                    changed |= ImGui::DragFloat("x##Scale", &scale.x, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("y##Scale", &scale.y, 1.0f);
                    ImGui::SameLine();
                    changed |= ImGui::DragFloat("z##Scale", &scale.z, 1.0f);

                    if (changed)
                        pSelectedEntity->GetRootSpatialComponent()->SetLocalTransformScale(scale);

                    ImGui::PopItemWidth();
                }
            }

            for (auto pComponent : pSelectedEntity->GetComponents())
            {
                ImGui::PushID(pComponent);

                auto pTypeInfo = pComponent->GetTypeInfo();

                if (ImGui::CollapsingHeader(pTypeInfo->GetPrettyName().c_str(), ImGuiTreeNodeFlags_AllowItemOverlap))
                {
                    ImGui::Indent();
                    m_entityComponentsSystemInspector.Draw(pTypeInfo, pComponent);
                    ImGui::Unindent();
                }

                if (ImGui::BeginPopupContextItem("context_popup", ImGuiPopupFlags_MouseButtonRight))
                {
                    if (ImGui::MenuItem("Remove Component", "", false, true))
                    {
                        pSelectedEntity->DestroyComponent(pComponent->GetID());
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            for (auto pSystem : pSelectedEntity->GetSystems())
            {
                ImGui::PushID(pSystem);

                auto pTypeInfo = pSystem->GetTypeInfo();

                if (ImGui::CollapsingHeader(pTypeInfo->GetPrettyName().c_str()))
                {
                    ImGui::Indent();
                    m_entityComponentsSystemInspector.Draw(pTypeInfo, pSystem);
                    ImGui::Unindent();
                }

                if (ImGui::BeginPopupContextItem("context_popup", ImGuiPopupFlags_MouseButtonRight))
                {
                    if (ImGui::MenuItem("Remove System", "", false, true))
                    {
                        pSelectedEntity->DestroySystem(pSystem->GetTypeInfo());
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
                auto& typeInfos = m_pTypeRegistryService->GetTypesInScope("SYSTEMS");
                for (auto& pTypeInfo : typeInfos)
                {
                    if (ImGui::Selectable(pTypeInfo->m_prettyName.c_str()))
                    {
                        pSelectedEntity->CreateSystem(pTypeInfo);
                    }
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("add_component_popup"))
            {
                auto& componentTypeInfos = m_pTypeRegistryService->GetTypesInScope("COMPONENTS");
                for (auto& pTypeInfo : componentTypeInfos)
                {
                    if (ImGui::Selectable(pTypeInfo->m_prettyName.c_str()))
                    {
                        auto pComponent = pTypeInfo->CreateTypeInstance<IComponent>();
                        pSelectedEntity->AddComponent(pComponent);
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
        ImGui::End();
    }

    // ------- State management
    // The editor's state is saved to disk and loaded back
    virtual void LoadState(nlohmann::json& json, const TypeRegistryService* m_pTypeRegistryService) {}
    virtual void SaveState(nlohmann::json& json) {}
};
} // namespace aln