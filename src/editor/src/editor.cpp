#include "editor.hpp"
#include "animation_graph/animation_graph_editor.hpp"

#include "misc/cpp/imgui_stdlib.h"

#include <assets/handle.hpp>
#include <common/colors.hpp>
#include <core/material.hpp>
#include <core/mesh.hpp>
#include <core/texture.hpp>
#include <reflection/services/type_registry_service.hpp>

#include <anim/animation_clip.hpp>

#include <entities/entity_descriptors.hpp>
#include <entities/entity_system.hpp>

#include <glm/vec3.hpp>

#include <config/path.h>

namespace aln
{

namespace editor
{
void SetImGuiContext(const EditorImGuiContext& context)
{
    ImGui::SetAllocatorFunctions(*context.m_pAllocFunc, *context.m_pFreeFunc, context.m_pUserData);
    ImGui::SetCurrentContext(context.m_pImGuiContext);

    ImNodes::SetCurrentContext(context.m_pImNodesContext);
    ImNodes::SetImGuiContext(context.m_pImGuiContext);
}
} // namespace editor

Editor::Editor(WorldEntity& worldEntity)
    : m_worldEntity(worldEntity),
      m_assetsBrowser(DEFAULT_ASSETS_DIR) {}

void Editor::Update(const vk::DescriptorSet& renderedSceneImageDescriptorSet, const UpdateContext& context)
{
    // TODO: Save service on initialization
    const auto pTypeRegistryService = context.GetService<TypeRegistryService>();

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
                if (ImGui::MenuItem("New Project"))
                {
                }
                if (ImGui::MenuItem("Open Project"))
                {
                }
                if (ImGui::MenuItem("Save..."))
                {
                    SaveScene();
                    SaveState();
                    // TODO: Save assets !
                }
                if (ImGui::MenuItem("Export..."))
                {
                    // TODO: Export a portable folder with everything we need to run a game
                    // Compile assets / Recompile paths
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Item");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Assets"))
            {
                if (ImGui::BeginMenu("Create..."))
                {
                    for (auto pFactory : m_assetWindowsFactory.m_factories)
                    {
                        if (ImGui::MenuItem(pFactory->m_assetEditorName.c_str()))
                        {
                            // TODO: Handle default asset name better
                            AssetID id = AssetID(std::string(DEFAULT_ASSETS_DIR) + "/default." + pFactory->m_supportedAssetType.ToString());
                            CreateAssetWindow(id, false);
                        }
                    }
                    ImGui::EndMenu();
                }
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
            ImGui::Text(fmt::format("{:.0f} FPS", 1.0f / context.GetDeltaTime()).c_str());
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

    // Windows
    m_assetsBrowser.Update(context);
    m_entityInspector.Update(context);

    for (auto& [id, pWindow] : m_assetWindows)
    {
        pWindow->Update(context);
    }

    // Process requests by child windows
    ResolveAssetWindowRequests();
}

void Editor::RecurseEntityTree(Entity* pEntity)
{
    ImGui::PushID(pEntity->GetID().ToString().c_str());
    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
    ImGuiTreeNodeFlags node_flags = base_flags;

    if (m_editorWindowContext.m_pSelectedEntity == pEntity)
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
        m_editorWindowContext.m_pSelectedEntity = pEntity;
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
            ImGui::Separator();
            if (ImGui::MenuItem("Remove Entity"))
            {
                if (m_editorWindowContext.m_pSelectedEntity == pEntity)
                {
                    m_editorWindowContext.m_pSelectedEntity = nullptr;
                }
                m_worldEntity.m_entityMap.RemoveEntity(pEntity);
            }
        }

        if (ImGui::MenuItem("Create Empty Entity", "", false, pEntity == nullptr))
        {
            auto pNewEntity = m_worldEntity.m_entityMap.CreateEntity("Entity");
        }

        ImGui::Separator();

        // Clipboard management
        // TODO: Handle keyboard shortcuts
        // TODO: Handle removing, copying, pasting, hierarchies of entities
        // TODO: Handle removing, copying, pasting, multiple selected entities
        if (pEntity != nullptr)
        {
            if (ImGui::MenuItem("Cut", "Ctrl + X"))
            {
                m_entityClipboard = EntityDescriptor(pEntity, m_pTypeRegistryService);
                if (m_editorWindowContext.m_pSelectedEntity == pEntity)
                {
                    m_editorWindowContext.m_pSelectedEntity = nullptr;
                }
                m_worldEntity.m_entityMap.RemoveEntity(pEntity);
            }

            if (ImGui::MenuItem("Copy", "Ctrl + C"))
            {
                m_entityClipboard = EntityDescriptor(pEntity, m_pTypeRegistryService);
            }
        }

        if (ImGui::MenuItem("Paste", "Ctrl + V", false, m_entityClipboard.IsValid()))
        {
            // TODO: Disable paste on non-spatial entities
            auto pNewEntity = m_worldEntity.m_entityMap.CreateEntity("");
            m_entityClipboard.InstanciateEntity(pNewEntity, m_pTypeRegistryService);
            if (pEntity != nullptr)
            {
                pNewEntity->SetParentEntity(pEntity);
            }
        }

        ImGui::EndPopup();
    }
}
} // namespace aln