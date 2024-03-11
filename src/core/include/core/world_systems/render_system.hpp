#pragma once

#include "../components/light.hpp"
#include "../components/skeletal_mesh_component.hpp"
#include "../components/static_mesh_component.hpp"
#include "../debug_render_states.hpp"

#include <common/containers/vector.hpp>
#include <common/drawing_context.hpp>
#include <common/hash_vector.hpp>
#include <entities/update_context.hpp>
#include <entities/world_system.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{

class CameraComponent;

class Entity;
class IComponent;
class SkeletalMesh;
class StaticMesh;
class RenderingService;

struct RenderData
{
    Vector<const SkeletalMeshComponent*> m_visibleSkeletalMeshComponents;
    Vector<const StaticMeshComponent*> m_visibleStaticMeshComponents;
    IDVector<Light*> m_lightComponents;
    const CameraComponent* m_pCameraComponent;
};

class GraphicsSystem : public IWorldSystem
{
    friend class SceneRenderer;

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

  public:
    /// @brief Per-frame GPU Resources
    struct GPUResources
    {
        GPUImage m_multisamplingImage;
        GPUImage m_depthImage;
        GPUImage m_resolveImage;
        vk::Framebuffer m_framebuffer;
    };

  private:
    /// @todo Rename
    RenderData m_renderData;
     
    // Per-world per-frame GPU resources. Acquired/Released from the global service
    Vector<GPUResources> m_gpuResources;

    // Viewport info
    /// @todo: Move to a specific Viewport class that get passed around
    float m_aspectRatio = 1.0f;

    // Debuging render state(s)
    LinesRenderState m_linesRenderState;

    // Registered components
    IDVector<SkeletalMeshRenderInstance> m_skeletalMeshRenderInstances;
    IDVector<StaticMeshRenderInstance> m_staticMeshRenderInstances;

    RenderingService* m_pRenderingService = nullptr;

  private:
    // -------------------------------------------------
    // System Methods
    // -------------------------------------------------
    void Shutdown(const ServiceProvider& serviceProvider) override;
    void Initialize(const ServiceProvider& serviceProvider) override;
    void Update(const UpdateContext& context) override;
    void RegisterComponent(const Entity* pEntity, IComponent* pComponent) override;
    void UnregisterComponent(const Entity* pEntity, IComponent* pComponent) override;
    const UpdatePriorities& GetUpdatePriorities() override;

    // Rendering calls
    void RenderDebugLines(vk::CommandBuffer& cb, DrawingContext& drawingContext);

  public:
    void SetRenderCamera(const CameraComponent* pCameraComponent)
    {
        assert(pCameraComponent != nullptr);
        m_renderData.m_pCameraComponent = pCameraComponent;
    }

    const RenderData& GetRenderData() const { return m_renderData; }

    bool IsReadyToRender() const { return m_renderData.m_pCameraComponent != nullptr; }
    const GPUResources& GetGPUResources() const;

};
} // namespace aln