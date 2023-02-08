#pragma once

#include <assert.h>
#include <concepts>
#include <filesystem>
#include <functional>
#include <map>
#include <typeindex>

#include <vulkan/vulkan.hpp>

#include <imgui.h>
#include <imnodes.h>

#include <IconsFontAwesome4.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <glm/gtc/random.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <fmt/core.h>

#include <common/memory.hpp>

#include <entities/component.hpp>
#include <entities/entity.hpp>
#include <entities/entity_descriptors.hpp>
#include <entities/spatial_component.hpp>
#include <entities/update_context.hpp>
#include <entities/world_entity.hpp>

#include <reflection/services/type_registry_service.hpp>

#include "animation_graph/animation_graph_editor.hpp"
#include "asset_editor_window.hpp"
#include "assets_browser.hpp"
#include "editor_window.hpp"
#include "types_editor.hpp"

namespace aln
{
struct EditorImGuiContext
{
    ImGuiContext* m_pImGuiContext = nullptr;
    ImNodesContext* m_pImNodesContext = nullptr;
    ImGuiMemAllocFunc m_pAllocFunc = nullptr;
    ImGuiMemFreeFunc m_pFreeFunc = nullptr;
    void* m_pUserData = nullptr;
};

namespace editor
{
/// @brief Set ImGui contexts and allocator functions in case reflection is in a separate library.
void SetImGuiContext(const EditorImGuiContext& context);
} // namespace editor

class Editor
{
  private:
    std::filesystem::path m_scenePath;

    WorldEntity& m_worldEntity;
    Entity* m_pSelectedEntity = nullptr;
    glm::vec3 m_currentEulerRotation; // Inspector's rotation is stored separately to avoid going back and forth between quat and euler
    EntityDescriptor m_entityClipboard;

    TypeEditorService m_typeEditorService;
    const TypeRegistryService* m_pTypeRegistryService;

    EditorWindowContext m_editorWindowContext;

    AssetEditorWindowsFactories m_assetWindowsFactory;
    std::map<AssetID, IAssetEditorWindow*> m_assetWindows;

    // TODO: Handle widget lifetime. For now they're always here !
    AssetsBrowser m_assetsBrowser;

    float m_scenePreviewWidth = 1.0f;
    float m_scenePreviewHeight = 1.0f;

    void EntityOutlinePopup(Entity* pEntity = nullptr);
    void RecurseEntityTree(Entity* pEntity);

    void ResolveAssetWindowRequests()
    {
        for (auto& assetID : m_editorWindowContext.m_requestedAssetWindowsDeletions)
        {
            RemoveAssetWindow(assetID);
        }
        m_editorWindowContext.m_requestedAssetWindowsDeletions.clear();

        for (auto& assetID : m_editorWindowContext.m_requestedAssetWindowsCreations)
        {
            // TODO: Find the right window type
            CreateAssetWindow(assetID);
        }
        m_editorWindowContext.m_requestedAssetWindowsCreations.clear();
    }

  public:
    Editor(WorldEntity& worldEntity);

    void CreateAssetWindow(const AssetID& id)
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
            it->second = m_assetWindowsFactory.CreateEditorWindow(id.GetAssetTypeID());
            it->second->Initialize(&m_editorWindowContext, id);
        }
    }

    void RemoveAssetWindow(const AssetID& id)
    {
        assert(id.IsValid());

        auto pWindow = m_assetWindows.extract(id).mapped();
        pWindow->Shutdown();
        aln::Delete(pWindow);
    }

    // -------------------
    // Editor Lifetime
    //--------------------
    void Initialize(ServiceProvider& serviceProvider, const std::filesystem::path& scenePath)
    {
        m_pTypeRegistryService = serviceProvider.GetService<TypeRegistryService>();

        // TODO: we could register type editor service to the provider here but for it shouldnt be required elsewhere
        m_editorWindowContext.m_pAssetService = serviceProvider.GetService<AssetService>();
        m_editorWindowContext.m_pTypeEditorService = &m_typeEditorService;
        m_assetWindowsFactory.RegisterFactory<AnimationGraphDefinition, AnimationGraphDefinitionEditorWindowFactory>();

        m_assetsBrowser.Initialize(&m_editorWindowContext);

        // TODO: Usability stuff: automatically load last used scene etc
        m_scenePath = scenePath;
        LoadScene();
        LoadState();
    }

    void Shutdown()
    {
        for (auto& [id, pWindow] : m_assetWindows)
        {
            pWindow->Shutdown();
            aln::Delete(pWindow);
        }
        m_assetWindows.clear();
    }

    const glm::vec2& GetScenePreviewSize() const { return {m_scenePreviewWidth, m_scenePreviewHeight}; }

    void SaveScene() const
    {
        EntityMapDescriptor mapDescriptor = EntityMapDescriptor(m_worldEntity.m_entityMap, *m_pTypeRegistryService);
        BinaryFileArchive archive(m_scenePath, IBinaryArchive::IOMode::Write);
        archive << mapDescriptor;
    }

    void LoadScene()
    {
        // TODO
        EntityMapDescriptor mapDescriptor;
        BinaryFileArchive archive(m_scenePath, IBinaryArchive::IOMode::Read);
        archive >> mapDescriptor;

        mapDescriptor.InstanciateEntityMap(m_worldEntity.m_entityMap, *m_pTypeRegistryService);
    }

    void SaveState() const
    {
        // TODO: Save the current state of the editor
        // including all subwindows
        for (auto& [assetID, pWindow] : m_assetWindows)
        {
        }
    }

    void LoadState()
    {
    }

    void Update(const vk::DescriptorSet& renderedSceneImageDescriptorSet, const UpdateContext& context);
};
} // namespace aln