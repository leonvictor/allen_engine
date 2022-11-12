#pragma once

#include "../debug_render_states.hpp"
#include "../drawing_context.hpp"

#include <graphics/resources/buffer.hpp>

#include <entities/components_registry.hpp>
#include <entities/update_context.hpp>
#include <entities/world_system.hpp>

#include <common/vertex.hpp>

#include <map>

namespace aln
{

class StaticMeshComponent;
class SkeletalMeshComponent;
class Light;
class Camera;

class Entity;
class IComponent;

namespace vkg::render
{
class IRenderer;
}

class GraphicsSystem : public IWorldSystem
{
    vkg::render::IRenderer* m_pRenderer = nullptr;

    ComponentsRegistry<StaticMeshComponent> m_staticMeshComponents;
    ComponentsRegistry<SkeletalMeshComponent> m_skeletalMeshComponents;
    ComponentsRegistry<Light> m_lightComponents;

    Camera* m_pCameraComponent = nullptr;

    // Viewport info
    /// @todo: Move to a specific Viewport class that get passed around
    float m_aspectRatio = 1.0f;

    // Lights use a shared buffer and descriptorSet, so it's held in the system
    /// @todo: Bundle in a specific class
    vkg::resources::Buffer m_lightsBuffer;
    vk::UniqueDescriptorSet m_lightsVkDescriptorSet;

    // Debuging render state(s)
    LinesRenderState m_linesRenderState;

    void CreateLightsDescriptorSet();
    void CreateLinesRenderContext();

    // -------------------------------------------------
    // System Methods
    // -------------------------------------------------
    void Shutdown() override;
    void Initialize() override;
    void Update(const UpdateContext& context) override;
    void RegisterComponent(const Entity* pEntity, IComponent* pComponent) override;
    void UnregisterComponent(const Entity* pEntity, IComponent* pComponent) override;
    const UpdatePriorities& GetUpdatePriorities() override;

    // Rendering calls
    void RenderSkeletalMeshes();
    void RenderDebugLines(vk::CommandBuffer& cb, DrawingContext& drawingContext);

  public:
    GraphicsSystem(vkg::render::IRenderer* pRenderer);
};
} // namespace aln