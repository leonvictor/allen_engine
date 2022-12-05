#pragma once

#include <assert.h>
#include <concepts>
#include <functional>
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
#include <reflection/reflection.hpp>

#include <entities/component.hpp>
#include <entities/entity.hpp>
#include <entities/spatial_component.hpp>
#include <entities/world_entity.hpp>

#include "animation_graph/animation_graph_editor.hpp"
#include "assets_browser.hpp"
#include "types_editor.hpp"

namespace aln::editor
{
struct EditorImGuiContext
{
    ImGuiContext* m_pImGuiContext = nullptr;
    ImNodesContext* m_pImNodesContext = nullptr;
    ImGuiMemAllocFunc m_pAllocFunc = nullptr;
    ImGuiMemFreeFunc m_pFreeFunc = nullptr;
    void* m_pUserData = nullptr;
};

/// @brief Set ImGui contexts and allocator functions in case reflection is in a separate library.
void SetImGuiContext(const EditorImGuiContext& context);

class Editor
{
  private:
    WorldEntity& m_worldEntity;
    Entity* m_pSelectedEntity = nullptr;
    glm::vec3 m_currentEulerRotation; // Inspector's rotation is stored separately to avoid going back and forth between quat and euler

    TypeEditorService m_typeEditorService;

    // TODO: Handle widget lifetime. For now they're always here !
    AnimationGraphEditor m_animationGraphEditor;
    AssetsBrowser m_assetsBrowser;

    float m_scenePreviewWidth = 1.0f;
    float m_scenePreviewHeight = 1.0f;

    void EntityOutlinePopup(Entity* pEntity = nullptr);
    void RecurseEntityTree(Entity* pEntity);

  public:
    Editor(WorldEntity& worldEntity);

    const glm::vec2& GetScenePreviewSize() const
    {
        return {m_scenePreviewWidth, m_scenePreviewHeight};
    }

    void DrawUI(const vk::DescriptorSet& renderedSceneImageDescriptorSet, float deltaTime);
};
} // namespace aln::editor