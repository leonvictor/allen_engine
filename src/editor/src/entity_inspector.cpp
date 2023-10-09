#include "entity_inspector.hpp"

#include "editor_window.hpp"

#include <entities/entity.hpp>
#include <entities/spatial_component.hpp>
#include <entities/world_entity.hpp>
#include <reflection/services/type_registry_service.hpp>

#include <IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_stdlib.h>

namespace aln
{
void EntityInspector::Initialize(EditorWindowContext* pEditorWindowContext)
{
    IEditorWindow::Initialize(pEditorWindowContext);
    m_pTypeRegistryService = pEditorWindowContext->m_pTypeRegistryService;
}

void EntityInspector::Shutdown()
{
    m_pTypeRegistryService = nullptr;
    IEditorWindow::Shutdown();
}

// TODO: Handle reordering the components hierarchy
void EntityInspector::DrawSpatialComponentsHierarchy(SpatialComponent* pRootComponent)
{
    ImGui::PushID(pRootComponent);

    const auto pTypeInfo = pRootComponent->GetTypeInfo();
    const auto isLeaf = pRootComponent->HasChildren();

    ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_None;
    if (isLeaf)
    {
        treeNodeFlags = treeNodeFlags | ImGuiTreeNodeFlags_Leaf;
    }

    auto nodeOpen = ImGui::TreeNodeEx(pTypeInfo->GetPrettyName().c_str(), treeNodeFlags);
    if (ImGui::IsItemClicked())
    {
        SetInspectedObject(pRootComponent);
    }

    if (nodeOpen)
    {
        for (auto& pChildComponent : pRootComponent->m_spatialChildren)
        {
            DrawSpatialComponentsHierarchy(pChildComponent);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void EntityInspector::Update(const UpdateContext& context)
{
    // Update selected entity
    auto pWorldEntity = GetWorldEntity();
    auto pSelectedEntity = GetSelectedEntity();
    if (pSelectedEntity != m_pEntity)
    {
        m_pEntity = pSelectedEntity;
    }

    // Inspector panel
    if (ImGui::Begin(ICON_FA_CIRCLE_INFO " Entity Inspector", nullptr) && m_pEntity != nullptr)
    {
        ImGui::PushID(m_pEntity->GetID().ToString().c_str());

        // Header
        ImGui::AlignTextToFramePadding();
        ImGui::Text(ICON_FA_CUBES);
        ImGui::SameLine();
        ImGui::InputText("", &m_pEntity->GetName());
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);

        if (m_pEntity->IsActivated())
        {
            ImGui::Text(ICON_FA_EYE);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Disable entity");
            }
            if (ImGui::IsItemClicked())
            {
                pWorldEntity->DeactivateEntity(m_pEntity);
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
                pWorldEntity->ActivateEntity(m_pEntity);
            }
        }

        // Spatial components hierarchy
        if (m_pEntity->IsSpatialEntity() && ImGui::CollapsingHeader("Spatial Components"))
        {
            auto pRootComponent = m_pEntity->GetRootSpatialComponent();
            DrawSpatialComponentsHierarchy(pRootComponent);
        }

        // Other components
        Vector<IComponent*> nonSpatialComponents;
        for (auto pComponent : m_pEntity->GetComponents())
        {
            auto pSpatialComponent = dynamic_cast<SpatialComponent*>(pComponent);
            if (pSpatialComponent == nullptr)
            {
                nonSpatialComponents.push_back(pComponent);
            }
        }

        if (!nonSpatialComponents.empty() && ImGui::CollapsingHeader("Components"))
        {
            for (auto pComponent : nonSpatialComponents)
            {
                ImGui::PushID(pComponent);

                auto pTypeInfo = pComponent->GetTypeInfo();
                if (ImGui::Selectable(pTypeInfo->GetPrettyName().c_str()))
                {
                    SetInspectedObject(pComponent);
                }

                if (ImGui::BeginPopupContextItem("context_popup", ImGuiPopupFlags_MouseButtonRight))
                {
                    if (ImGui::MenuItem("Remove Component", "", false, true))
                    {
                        m_pEntity->DestroyComponent(pComponent->GetID());
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }
        }

        // Systems
        if (!m_pEntity->GetSystems().empty() && ImGui::CollapsingHeader("Systems"))
        {
            for (auto pSystem : m_pEntity->GetSystems())
            {
                ImGui::PushID(pSystem);
                ImGui::Indent();

                auto pTypeInfo = pSystem->GetTypeInfo();

                if (ImGui::Selectable(pTypeInfo->GetPrettyName().c_str()))
                {
                    SetInspectedObject(pSystem);
                }

                if (ImGui::BeginPopupContextItem("context_popup", ImGuiPopupFlags_MouseButtonRight))
                {
                    if (ImGui::MenuItem("Remove System", "", false, true))
                    {
                        m_pEntity->DestroySystem(pSystem->GetTypeInfo());
                    }
                    ImGui::EndPopup();
                }

                ImGui::Unindent();
                ImGui::PopID();
            }
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
                    m_pEntity->CreateSystem(pTypeInfo);
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
                    m_pEntity->AddComponent(pComponent);
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
    ImGui::End();
}
} // namespace aln