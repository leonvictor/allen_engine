#pragma once

#include <graphics/resources/buffer.hpp>

#include <entities/components_registry.hpp>
#include <entities/object_model.hpp>
#include <entities/world_system.hpp>

#include <map>

namespace aln
{

class StaticMeshComponent;
class SkeletalMeshComponent;
class Light;
class Camera;

namespace entities
{
class Entity;
class IComponent;
} // namespace entities

namespace vkg::render
{
class IRenderer;
}

class GraphicsSystem : public entities::IWorldSystem
{
    vkg::render::IRenderer* m_pRenderer = nullptr;

    aln::entities::ComponentsRegistry<StaticMeshComponent> m_staticMeshComponents;
    aln::entities::ComponentsRegistry<SkeletalMeshComponent> m_skeletalMeshComponents;
    aln::entities::ComponentsRegistry<Light> m_lightComponents;

    Camera* m_pCameraComponent = nullptr;

    // Lights use a shared buffer and descriptorSet, so it's held in the system
    vkg::resources::Buffer m_lightsBuffer;
    vk::UniqueDescriptorSet m_lightsVkDescriptorSet;

    void CreateLightsDescriptorSet();

    // -------------------------------------------------
    // System Methods
    // -------------------------------------------------
    void Shutdown() override;
    void Initialize() override;
    void Update(const entities::UpdateContext& context) override;
    void RegisterComponent(const entities::Entity* pEntity, entities::IComponent* pComponent) override;
    void UnregisterComponent(const entities::Entity* pEntity, entities::IComponent* pComponent) override;
    const entities::UpdatePriorities& GetUpdatePriorities() override;

    // Rendering calls
    void RenderSkeletalMeshes();

  public:
    GraphicsSystem(vkg::render::IRenderer* pRenderer);
};
} // namespace aln