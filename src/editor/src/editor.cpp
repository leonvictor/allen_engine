#include "editor.hpp"

#include "assets/animation_clip_workspace.hpp"
#include "assets/animation_graph/animation_graph_workspace.hpp"

#include <assets/asset_service.hpp>
#include <common/memory.hpp>
#include <config/path.h>
#include <core/components/camera.hpp>
#include <core/entity_systems/camera_controller.hpp>
#include <core/renderers/scene_renderer.hpp>
#include <core/world_systems/render_system.hpp>
#include <entities/world_entity.hpp>
#include <reflection/services/type_registry_service.hpp>

#include <IconsFontAwesome6.h>
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>

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

    if (ImGui::Begin(ICON_FA_GLOBE " Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu(ICON_FA_VIDEO " " ICON_FA_CARET_DOWN))
            {
                auto availableCameras = GetAllComponentsOfType<CameraComponent>();
                for (auto camera : availableCameras)
                {
                    if (ImGui::MenuItem((camera.m_pOwningEntity->GetName() + "::" + camera.m_pComponent->GetTypeInfo()->GetPrettyName()).c_str()))
                    {
                        auto pRenderingSystem = m_worldEntity.GetSystem<GraphicsSystem>();
                        pRenderingSystem->SetRenderCamera(static_cast<CameraComponent*>(camera.m_pComponent));
                    }
                    // TODO: Tooltip with component ID to disambiguate an entity having mutliple cameras of the same type
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

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
    m_propertiesInspector.Update(context);

    for (auto& [id, pWindow] : m_assetWindows)
    {
        pWindow->Update(context);
    }

    // Process requests by child windows
    ResolveAssetWindowRequests();
}

void Editor::RecurseEntityTree(Entity* pEntity)
{
    ImGui::PushID(pEntity);
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

void Editor::EntityOutlinePopup(Entity* pContextEntity)
{
    if (ImGui::BeginPopupContextItem("entity_outline_popup", ImGuiPopupFlags_MouseButtonRight))
    {
        if (pContextEntity != nullptr)
        {
            ImGui::Text(pContextEntity->GetID().ToString().c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Remove Entity"))
            {
                if (m_editorWindowContext.m_pSelectedEntity == pContextEntity)
                {
                    m_editorWindowContext.m_pSelectedEntity = nullptr;
                }
                m_worldEntity.m_entityMap.RemoveEntity(pContextEntity);
            }
        }

        if (ImGui::MenuItem("Create Empty Entity", "", false, pContextEntity == nullptr))
        {
            auto pNewEntity = m_worldEntity.m_entityMap.CreateEntity("Entity");
        }

        ImGui::Separator();

        // Clipboard management
        // TODO: Handle keyboard shortcuts
        // TODO: Handle removing, copying, pasting, hierarchies of entities
        // TODO: Handle removing, copying, pasting, multiple selected entities
        if (pContextEntity != nullptr)
        {
            if (ImGui::MenuItem("Cut", "Ctrl + X"))
            {
                m_entityClipboard = EntityDescriptor(pContextEntity, m_pTypeRegistryService);
                if (m_editorWindowContext.m_pSelectedEntity == pContextEntity)
                {
                    m_editorWindowContext.m_pSelectedEntity = nullptr;
                }
                m_worldEntity.m_entityMap.RemoveEntity(pContextEntity);
            }

            if (ImGui::MenuItem("Copy", "Ctrl + C"))
            {
                m_entityClipboard = EntityDescriptor(pContextEntity, m_pTypeRegistryService);
            }
        }

        if (ImGui::MenuItem("Paste", "Ctrl + V", false, m_entityClipboard.IsValid()))
        {
            // TODO: Disable paste on non-spatial entities
            auto pNewEntity = m_worldEntity.m_entityMap.CreateEntity("");
            m_entityClipboard.InstanciateEntity(pNewEntity, m_pTypeRegistryService);
            if (pContextEntity != nullptr)
            {
                pNewEntity->SetParentEntity(pContextEntity);
            }
        }

        ImGui::EndPopup();
    }
}

void Editor::CreateAssetWindow(const AssetID& id, bool readAssetFile)
{
    assert(id.IsValid());

    // TODO: Wouldn't be necessary with a default asset editor window
    if (!m_assetWindowsFactory.IsTypeRegistered(id.GetAssetTypeID()))
    {
        return;
    }

    auto [it, inserted] = m_assetWindows.try_emplace(id, nullptr);
    if (inserted)
    {
        it->second = m_assetWindowsFactory.CreateWorkspace(id.GetAssetTypeID());
        it->second->Initialize(&m_editorWindowContext, id, readAssetFile);
    }
}

void Editor::RemoveAssetWindow(const AssetID& id)
{
    assert(id.IsValid());

    auto it = m_assetWindows.find(id);
    assert(it != m_assetWindows.end());
    it->second->Shutdown();
    aln::Delete(it->second);
    m_assetWindows.erase(it);
}

void Editor::ResolveAssetWindowRequests()
{
    for (auto& assetID : m_editorWindowContext.m_requestedAssetWindowsDeletions)
    {
        RemoveAssetWindow(assetID);
    }
    m_editorWindowContext.m_requestedAssetWindowsDeletions.clear();

    for (auto& assetID : m_editorWindowContext.m_requestedAssetWindowsCreations)
    {
        // TODO: Find the right window type
        CreateAssetWindow(assetID, true);
    }
    m_editorWindowContext.m_requestedAssetWindowsCreations.clear();
}

void Editor::Initialize(ServiceProvider& serviceProvider, const std::filesystem::path& scenePath)
{
    m_pTypeRegistryService = serviceProvider.GetService<TypeRegistryService>();

    // TODO: we could register type editor service to the provider here but for it shouldnt be required elsewhere
    m_editorWindowContext.m_pAssetService = serviceProvider.GetService<AssetService>();
    m_editorWindowContext.m_pTypeRegistryService = serviceProvider.GetService<TypeRegistryService>();
    m_editorWindowContext.m_pWorldEntity = &m_worldEntity;

    m_assetWindowsFactory.RegisterFactory<AnimationGraphDefinition, AnimationGraphDefinitionEditorWindowFactory>("Animation Graph");
    // TODO: Animation clips can't be created from scratch in the editor. Remove them from the list
    m_assetWindowsFactory.RegisterFactory<AnimationClip, AnimationClipEditorWindowFactory>("Animation Clip");

    m_assetsBrowser.Initialize(&m_editorWindowContext);
    m_entityInspector.Initialize(&m_editorWindowContext);
    m_propertiesInspector.Initialize(&m_editorWindowContext);

    m_scenePath = scenePath;

    // TODO: Usability stuff: automatically load last used scene etc
    if (std::filesystem::exists(scenePath))
    {
        LoadScene();
        LoadState();
    }
    else
    {
        m_pCamera = aln::New<CameraComponent>();
        m_pEditorEntity = m_worldEntity.m_entityMap.CreateEntity("Editor");
        m_pEditorEntity->AddComponent(m_pCamera);
        m_pEditorEntity->CreateSystem<EditorCameraController>();
    }
}

void Editor::Shutdown()
{
    for (auto& [id, pWindow] : m_assetWindows)
    {
        pWindow->Shutdown();
        aln::Delete(pWindow);
    }
    m_assetWindows.clear();

    m_assetsBrowser.Shutdown();
    m_entityInspector.Shutdown();
    m_propertiesInspector.Shutdown();

    ReflectedTypeEditor::Shutdown();
}

void Editor::SaveScene() const
{
    EntityMapDescriptor mapDescriptor = EntityMapDescriptor(m_worldEntity.m_entityMap, *m_pTypeRegistryService);
    BinaryFileArchive archive(m_scenePath, IBinaryArchive::IOMode::Write);
    archive << mapDescriptor;
}

void Editor::LoadScene()
{
    // TODO
    EntityMapDescriptor mapDescriptor;
    BinaryFileArchive archive(m_scenePath, IBinaryArchive::IOMode::Read);
    archive >> mapDescriptor;

    mapDescriptor.InstanciateEntityMap(m_worldEntity.m_entityMap, m_worldEntity.m_loadingContext, *m_pTypeRegistryService);
}

void Editor::SaveState() const
{
    // TODO: Save the current state of the editor
    // including all subwindows
    for (auto& [assetID, pWindow] : m_assetWindows)
    {
    }
}

void Editor::LoadState()
{
}
} // namespace aln