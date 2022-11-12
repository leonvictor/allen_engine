#pragma once

#include <assert.h>
#include <concepts>
#include <functional>
#include <typeindex>

#include <vulkan/vulkan.hpp>

#include <imgui.h>

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

namespace aln::editor
{
// ImGui methods to set the context and allocator functions in case reflection is in a separate library.
void SetImGuiContext(ImGuiContext* pContext);
void SetImGuiAllocatorFunctions(ImGuiMemAllocFunc* pAllocFunc, ImGuiMemFreeFunc* pFreeFunc, void** pUserData);

class Editor
{
  private:
    entities::WorldEntity& m_worldEntity;
    entities::Entity* m_pSelectedEntity = nullptr;
    glm::vec3 m_currentEulerRotation; // Inspector's rotation is stored separately to avoid going back and forth between quat and euler

    float m_scenePreviewWidth = 1.0f;
    float m_scenePreviewHeight = 1.0f;

    std::unordered_map<std::type_index, std::function<void(void*, const char*)>> m_displayFuncs;

    template <typename T>
    void RegisterType();

    template <typename T>
    void Display(void* obj, const char* label);
    void Display(std::type_index typeIndex, void* obj, const char* label);

    template <typename T>
    void InInspector(T* obj, const char* label) { Display<T>(obj, label); }

    void DisplayTypeStruct(const reflect::TypeDescriptor_Struct* pType, void* obj);
    void EntityOutlinePopup(entities::Entity* pEntity = nullptr);
    void RecurseEntityTree(entities::Entity* pEntity);

  public:
    Editor(entities::WorldEntity& worldEntity);


    const glm::vec2& GetScenePreviewSize() const
    {
        return {m_scenePreviewWidth, m_scenePreviewHeight};
    }

    void DrawUI(const vk::DescriptorSet& renderedSceneImageDescriptorSet, float deltaTime);
};
} // namespace aln::editor