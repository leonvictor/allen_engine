#pragma once

#include "../debug_render_states.hpp"
#include "../drawing_context.hpp"
#include "../skeletal_mesh.hpp"
#include "../static_mesh.hpp"

#include <graphics/resources/buffer.hpp>

#include <entities/components_registry.hpp>
#include <entities/update_context.hpp>
#include <entities/world_system.hpp>

#include <common/hash_vector.hpp>
#include <common/vertex.hpp>

#include <map>
#include <vector>
#include <vulkan/vulkan.hpp>

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

    Camera* m_pCameraComponent = nullptr;
    vkg::resources::Buffer m_cameraUBO;

    // Viewport info
    /// @todo: Move to a specific Viewport class that get passed around
    float m_aspectRatio = 1.0f;

    // Lights use a shared buffer and descriptorSet, so it's held in the system
    /// @todo: Bundle in a specific class
    /// @todo: ComponentsRegistry is not necessary
    ComponentsRegistry<Light> m_lightComponents;
    vkg::resources::Buffer m_lightsBuffer;
    vk::UniqueDescriptorSet m_lightsVkDescriptorSet;

    // Debuging render state(s)
    LinesRenderState m_linesRenderState;

    // Mesh components are grouped by mesh instance so that we can have one descriptor per mesh instance
    // This also allows us to decouple rendering stuff from the base mesh classes
    /// @todo The state could be held by the renderer so that it could be swapped easily. Not useful yet tho !
    struct SkeletalMeshRenderInstance
    {
        const SkeletalMesh* m_pMesh;
        std::vector<SkeletalMeshComponent*> m_components;
        vkg::resources::Buffer m_skinningBuffer;
        vk::UniqueDescriptorSet m_descriptorSet;

        SkeletalMeshRenderInstance(vkg::Device* pDevice, const SkeletalMesh* pMesh, vkg::resources::Buffer* pUniformBuffer);
        uint32_t GetID() const { return m_pMesh->GetID(); }
    };

    struct StaticMeshRenderInstance
    {
        const StaticMesh* m_pMesh;
        std::vector<StaticMeshComponent*> m_components;
        vk::UniqueDescriptorSet m_descriptorSet;

        StaticMeshRenderInstance(vkg::Device* pDevice, const StaticMesh* pMesh, vkg::resources::Buffer* pUniformBuffer);
        uint32_t GetID() const { return m_pMesh->GetID(); }
    };

    IDVector<SkeletalMeshRenderInstance> m_SkeletalMeshRenderInstances;
    IDVector<StaticMeshRenderInstance> m_StaticMeshRenderInstances;

    void CreateLightsDescriptorSet();

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
    void RenderDebugLines(vk::CommandBuffer& cb, DrawingContext& drawingContext);

  public:
    GraphicsSystem(vkg::render::IRenderer* pRenderer);
};
} // namespace aln