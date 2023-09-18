#pragma once

#include "asset_editor_workspace.hpp"
#include "assets/animation_graph/animation_graph_workspace.hpp"
#include "assets_browser.hpp"
#include "editor_window.hpp"
#include "entity_inspector.hpp"
#include "reflected_types/reflected_type_editor.hpp"

#include <common/containers/hash_map.hpp>
#include <entities/entity_descriptors.hpp>
#include <entities/world_entity.hpp>

#include <common/maths/vec2.hpp>

#include <filesystem>

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
class CameraComponent;
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
  public:
    struct ComponentSearchResult
    {
        Entity* m_pOwningEntity = nullptr;
        IComponent* m_pComponent = nullptr;
    };

  private:
    std::filesystem::path m_scenePath;

    WorldEntity& m_worldEntity;
    Entity* m_pEditorEntity = nullptr;
    CameraComponent* m_pCamera = nullptr;

    EntityDescriptor m_entityClipboard;

    const TypeRegistryService* m_pTypeRegistryService = nullptr;

    EditorWindowContext m_editorWindowContext;

    AssetEditorWindowsFactories m_assetWindowsFactory;
    HashMap<AssetID, IAssetWorkspace*> m_assetWindows;

    // TODO: Handle widget lifetime. For now they're always here !
    AssetsBrowser m_assetsBrowser;
    EntityInspector m_entityInspector;

    float m_scenePreviewWidth = 1.0f;
    float m_scenePreviewHeight = 1.0f;

    void EntityOutlinePopup(Entity* pEntity = nullptr);
    void RecurseEntityTree(Entity* pEntity);

    void ResolveAssetWindowRequests();

    template <typename T>
    Vector<ComponentSearchResult> GetAllComponentsOfType()
    {
        Vector<ComponentSearchResult> results;
        auto pTypeInfo = T::GetStaticTypeInfo();
        for (auto pEntity : m_worldEntity.GetEntities())
        {
            for (auto pComponent : pEntity->GetComponents())
            {
                if (pComponent->GetTypeInfo()->IsDerivedFrom(pTypeInfo->GetTypeID()))
                {
                    results.push_back({pEntity, pComponent});
                }
            }
        }
        return results;
    }

  public:
    Editor(WorldEntity& worldEntity);

    // -------------------
    // Editor Lifetime
    //--------------------
    void Initialize(ServiceProvider& serviceProvider, const std::filesystem::path& scenePath);
    void Shutdown();
    void Update(const vk::DescriptorSet& renderedSceneImageDescriptorSet, const UpdateContext& context);

    const Vec2& GetScenePreviewSize() const { return {m_scenePreviewWidth, m_scenePreviewHeight}; }

    void CreateAssetWindow(const AssetID& id, bool readAssetFile);
    void RemoveAssetWindow(const AssetID& id);

    void SaveScene() const;
    void LoadScene();

    void SaveState() const;
    void LoadState();
};
} // namespace aln