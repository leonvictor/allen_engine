#include "properties_window.hpp"

#include "editor_window.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <entities/entity.hpp>
#include <entities/spatial_component.hpp>
#include <entities/world_entity.hpp>
#include <reflection/services/type_registry_service.hpp>

#include <IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_stdlib.h>

namespace aln
{
void PropertiesWindow::Initialize(EditorWindowContext* pEditorWindowContext)
{
    IEditorWindow::Initialize(pEditorWindowContext);

    m_propertyEditingStartedEventID = m_reflectedTypeEditor.OnTypeEditingStarted().BindListener([this](const TypeEditedEventDetails& eventDetails)
        { BeginComponentEditing(eventDetails); });
    m_propertyEditingCompletedEventID = m_reflectedTypeEditor.OnTypeEditingCompleted().BindListener([this](const TypeEditedEventDetails& eventDetails)
        { EndComponentEditing(eventDetails); });
}

void PropertiesWindow::Shutdown()
{
    m_reflectedTypeEditor.OnTypeEditingStarted().UnbindListener(m_propertyEditingStartedEventID);
    m_reflectedTypeEditor.OnTypeEditingCompleted().UnbindListener(m_propertyEditingCompletedEventID);

    m_reflectedTypeEditor.Shutdown();

    m_pTypeRegistryService = nullptr;
    IEditorWindow::Shutdown();
}

void PropertiesWindow::Update(const UpdateContext& context)
{
    auto pWorldEntity = GetWorldEntity();
    auto pSelectedEntity = GetSelectedEntity();
    auto pInspectedObject = GetInspectedObject();

    if (pInspectedObject != m_pInspectedObject)
    {
        m_pInspectedObject = pInspectedObject;
        
        auto pSpatialComponent = dynamic_cast<SpatialComponent*>(m_pInspectedObject);
        if (pSpatialComponent != nullptr)
        {
            m_currentEulerRotation = pSpatialComponent->GetLocalTransform().GetRotationEuler();
        }
    }

    // Inspector panel
    if (ImGui::Begin(ICON_FA_CIRCLE_INFO " Properties", nullptr) && pInspectedObject != nullptr)
    {
        ImGui::PushID(m_pInspectedObject);

        const auto pInspectedTypeInfo = m_pInspectedObject->GetTypeInfo();

        auto pSpatialComponent = dynamic_cast<SpatialComponent*>(m_pInspectedObject);
        if (pSpatialComponent != nullptr)
        {
            // TODO: Transform widget
            ImGui::PushItemWidth(60);

            // TODO: vec3 might deserve a helper function to create ui for the 3 components...
            // Position
            bool changed = false;
            auto pos = pSpatialComponent->GetLocalTransform().GetTranslation();
            ImGui::Text("Position");
            changed |= ImGui::DragFloat("x##Position", &pos.x, 1.0f);
            ImGui::SameLine();
            changed |= ImGui::DragFloat("y##Position", &pos.y, 1.0f);
            ImGui::SameLine();
            changed |= ImGui::DragFloat("z##Position", &pos.z, 1.0f);

            if (changed)
            {
                pSpatialComponent->SetLocalTransformPosition(pos);
            }

            // Rotation
            changed = false;
            ImGui::Text("Rotation");
            changed |= ImGui::DragFloat("x##Rotation", &m_currentEulerRotation.x, 1.0f);
            ImGui::SameLine();
            changed |= ImGui::DragFloat("y##Rotation", &m_currentEulerRotation.y, 1.0f);
            ImGui::SameLine();
            changed |= ImGui::DragFloat("z##Rotation", &m_currentEulerRotation.z, 1.0f);

            if (changed)
            {
                pSpatialComponent->SetLocalTransformRotationEuler(m_currentEulerRotation);
            }

            // Scale
            auto scale = pSpatialComponent->GetLocalTransform().GetScale();
            changed = false;

            ImGui::Text("Scale");
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            if (m_uniformScale)
            {
                ImGui::Text(ICON_FA_LINK);
            }
            else
            {
                ImGui::Text(ICON_FA_LINK_SLASH);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Enforce uniform scale by linking components together.");
            }
            if (ImGui::IsItemClicked())
            {
                m_uniformScale = !m_uniformScale;
            }

            if (ImGui::DragFloat("x##Scale", &scale.x, 1.0f))
            {
                if (m_uniformScale)
                {
                    scale.y = scale.x;
                    scale.z = scale.x;
                }
                changed = true;
            }
            ImGui::SameLine();

            if (ImGui::DragFloat("y##Scale", &scale.y, 1.0f))
            {
                if (m_uniformScale)
                {
                    scale.x = scale.y;
                    scale.z = scale.y;
                }
                changed = true;
            }
            ImGui::SameLine();

            if (ImGui::DragFloat("z##Scale", &scale.z, 1.0f))
            {
                if (m_uniformScale)
                {
                    scale.x = scale.z;
                    scale.y = scale.z;
                }
                changed = true;
            }

            if (changed)
            {
                pSpatialComponent->SetLocalTransformScale(scale);
            }

            ImGui::PopItemWidth();
        }

        m_reflectedTypeEditor.Draw(pInspectedTypeInfo, m_pInspectedObject);

        ImGui::PopID();
    }
    ImGui::End();
}

void PropertiesWindow::BeginComponentEditing(const TypeEditedEventDetails& editingEventDetails)
{
    if (editingEventDetails.m_action == TypeEditedEventDetails::Action::EditRequiresReload)
    {
        auto pWorldEntity = GetWorldEntity();
        auto pComponent = dynamic_cast<IComponent*>(editingEventDetails.m_pEditedTypeInstance);

        pWorldEntity->StartComponentEditing(pComponent);
    }
}
void PropertiesWindow::EndComponentEditing(const TypeEditedEventDetails& editingEventDetails)
{
}
} // namespace aln