#pragma once

#include "assets/animation_graph/animation_graph_workspace.hpp"
#include "asset_editor_workspace.hpp"
#include "assets_browser.hpp"
#include "editor_window.hpp"
#include "entity_inspector.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <entities/entity_descriptors.hpp>

#include <glm/vec2.hpp>

#include <filesystem>
#include <map>

namespace vk
{
class DescriptorSet;
}

struct ImNodesContext;
struct ImGuiContext;
typedef void* (*ImGuiMemAllocFunc)(size_t, void*);
typedef void (*ImGuiMemFreeFunc)(void*, void*);

namespace aln
{
class Entity;
class Camera;
class WorldEntity;
class TypeRegistryService;
class ServiceProvider;

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
    Entity* m_pEditorEntity = nullptr;
    Camera* m_pCamera = nullptr;

    EntityDescriptor m_entityClipboard;

    const TypeRegistryService* m_pTypeRegistryService = nullptr;

    EditorWindowContext m_editorWindowContext;

    AssetEditorWindowsFactories m_assetWindowsFactory;
    std::map<AssetID, IAssetWorkspace*> m_assetWindows;

    // TODO: Handle widget lifetime. For now they're always here !
    AssetsBrowser m_assetsBrowser;
    EntityInspector m_entityInspector;

    float m_scenePreviewWidth = 1.0f;
    float m_scenePreviewHeight = 1.0f;

    void EntityOutlinePopup(Entity* pEntity = nullptr);
    void RecurseEntityTree(Entity* pEntity);

    void ResolveAssetWindowRequests();

  public:
    Editor(WorldEntity& worldEntity);

    // -------------------
    // Editor Lifetime
    //--------------------
    void Initialize(ServiceProvider& serviceProvider, const std::filesystem::path& scenePath);
    void Shutdown();
    void Update(const vk::DescriptorSet& renderedSceneImageDescriptorSet, const UpdateContext& context);

    const glm::vec2& GetScenePreviewSize() const { return {m_scenePreviewWidth, m_scenePreviewHeight}; }

    void CreateAssetWindow(const AssetID& id, bool readAssetFile);
    void RemoveAssetWindow(const AssetID& id);

    void SaveScene() const;
    void LoadScene();

    void SaveState() const;
    void LoadState();

};
} // namespace aln