#pragma once

#include "../debug_render_states.hpp"
#include "../drawing_context.hpp"
#include "../skeletal_mesh.hpp"
#include "../static_mesh.hpp"

#include <graphics/resources/buffer.hpp>

#include "../components/light.hpp"
#include "../components/skeletal_mesh_component.hpp"
#include "../components/static_mesh_component.hpp"

#include <entities/update_context.hpp>
#include <entities/world_system.hpp>

#include <common/hash_vector.hpp>
#include <common/vertex.hpp>

#include <map>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace aln
{

class Camera;

class Entity;
class IComponent;

class GraphicsSystem : public IWorldSystem
{
    // Mesh components are grouped by mesh instance so that we can have one descriptor per mesh instance
    // This also allows us to decouple rendering stuff from the base mesh classes
    struct SkeletalMeshRenderInstance
    {
        const SkeletalMesh* m_pMesh;
        IDVector<SkeletalMeshComponent*> m_components;

        SkeletalMeshRenderInstance(const SkeletalMesh* pMesh) : m_pMesh(pMesh) {}
        uint32_t GetID() const { return m_pMesh->GetID(); }
    };

    struct StaticMeshRenderInstance
    {
        const StaticMesh* m_pMesh;
        IDVector<StaticMeshComponent*> m_components;

        StaticMeshRenderInstance(const StaticMesh* pMesh) : m_pMesh(pMesh) {}
        uint32_t GetID() const { return m_pMesh->GetID(); }
    };

    SceneRenderer* m_pRenderer = nullptr;

    // Viewport info
    /// @todo: Move to a specific Viewport class that get passed around
    float m_aspectRatio = 1.0f;

    // Debuging render state(s)
    LinesRenderState m_linesRenderState;

    // Registered components
    Camera* m_pCameraComponent = nullptr;
    IDVector<SkeletalMeshRenderInstance> m_skeletalMeshRenderInstances;
    IDVector<StaticMeshRenderInstance> m_staticMeshRenderInstances;
    IDVector<Light*> m_lightComponents;

    std::vector<const SkeletalMeshComponent*> m_visibleSkeletalMeshComponents;
    std::vector<const StaticMeshComponent*> m_visibleStaticMeshComponents;

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
    GraphicsSystem(SceneRenderer* pRenderer) : m_pRenderer(pRenderer) {}
};
} // namespace aln