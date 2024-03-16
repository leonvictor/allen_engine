#include "world_systems/world_rendering_system.hpp"

#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "renderers/world_renderer.hpp"
#include "services/rendering_service.hpp"

#include <common/maths/matrix4x4.hpp>
#include <entities/entity.hpp>
#include <entities/update_context.hpp>
#include <entities/world_system.hpp>
#include <graphics/rendering/renderer.hpp>

#include <tracy/Tracy.hpp>

namespace aln
{

void WorldRenderingSystem::RenderDebugLines(vk::CommandBuffer& cb, DrawingContext& drawingContext)
{
    const auto& vertexBuffer = drawingContext.m_vertices;
    auto vertexCount = vertexBuffer.size();

    // todo: Aggregate line data coming from multiple threads

    // TODO: Profile with a staging buffer
    // but vk buffer copy is not allowed inside a render pass
    // GPUBuffer stagingBuffer(m_pRenderer->GetDevice(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexBuffer);
    // m_linesRenderState.m_stagingBuffer.Map();
    // m_linesRenderState.m_stagingBuffer.Copy(vertexBuffer);
    // m_linesRenderState.m_stagingBuffer.Unmap();

    // m_linesRenderState.m_stagingBuffer.CopyTo(cb, m_linesRenderState.m_vertexBuffer);

    // Update UBO
    LinesRenderState::UBO ubo;
    ubo.m_viewProjectionMatrix = m_renderData.m_pCameraComponent->GetViewProjectionMatrix(m_aspectRatio);

    m_linesRenderState.m_viewProjectionUBO.Copy(ubo);

    // Update vertex buffer
    m_linesRenderState.m_vertexBuffer.Copy(vertexBuffer);

    // TODO: Handle a max number of lines per draw call
    // Split the buffer accordingly and draw each part sequentially

    auto& pipeline = m_linesRenderState.m_pipeline;
    pipeline.Bind(cb);

    pipeline.BindDescriptorSet(cb, m_linesRenderState.m_descriptorSet, 0);
    cb.bindVertexBuffers(0, m_linesRenderState.m_vertexBuffer.GetVkBuffer(), (vk::DeviceSize) 0);
    cb.draw(vertexBuffer.size(), 1, 0, 0);
}

void WorldRenderingSystem::Shutdown(const ServiceProvider& serviceProvider)
{
    // Notify the rendering service that we can release this world's resources
    m_pRenderingService->ReleaseWorldGPUResources(m_gpuResources);
    m_pRenderingService = nullptr;

    // TODO
    // m_linesRenderState.Shutdown();
}

void WorldRenderingSystem::Initialize(const ServiceProvider& serviceProvider)
{
    // Debug resources
    // TODO: Rework line debugging
    // m_linesRenderState.Initialize(m_pRenderer->GetDevice(), m_pRenderer);

    // Acquire per-world graphics ressources from the rendering service
    m_pRenderingService = serviceProvider.GetService<RenderingService>();
    m_pRenderingService->AcquireWorldGPUResources(m_gpuResources);
}

void WorldRenderingSystem::Update(const UpdateContext& context)
{
    if (context.GetUpdateStage() != UpdateStage::FrameEnd)
    {
        return;
    }

    ZoneScoped;

    assert(m_renderData.m_pCameraComponent != nullptr);
    if (!m_renderData.m_pCameraComponent->IsInitialized())
    {
        return; // Camera is the only necessary component, return if it's not loaded yet
    }

    // Update viewport info
    m_aspectRatio = context.GetDisplayWidth() / context.GetDisplayHeight();

    m_renderData.m_visibleStaticMeshComponents.clear();
    for (auto& meshInstance : m_staticMeshRenderInstances)
    {
        for (const auto& pStaticMeshComponent : meshInstance.m_components)
        {
            // TODO: Cull
            m_renderData.m_visibleStaticMeshComponents.push_back(pStaticMeshComponent);
        }
    }

    m_renderData.m_visibleSkeletalMeshComponents.clear();
    for (auto& meshInstance : m_skeletalMeshRenderInstances)
    {
        for (auto pSkeletalMeshComponent : meshInstance.m_components)
        {
            // TODO: Cull
            pSkeletalMeshComponent->UpdateSkinningTransforms();
            m_renderData.m_visibleSkeletalMeshComponents.push_back(pSkeletalMeshComponent);
        }
    }
}

void WorldRenderingSystem::RegisterComponent(const Entity* pEntity, IComponent* pComponent)
{
    // Set the first registered camera as the active one
    auto pCamera = dynamic_cast<CameraComponent*>(pComponent);
    if (m_renderData.m_pCameraComponent == nullptr && pCamera != nullptr)
    {
        m_renderData.m_pCameraComponent = pCamera;
        return;
    }

    auto pStaticMeshComponent = dynamic_cast<StaticMeshComponent*>(pComponent);
    if (pStaticMeshComponent != nullptr)
    {
        auto& meshInstance = m_staticMeshRenderInstances.TryEmplace(pStaticMeshComponent->GetMesh()->GetID(), pStaticMeshComponent->GetMesh());
        meshInstance.m_components.PushBack(pStaticMeshComponent);
        return;
    }

    auto pSkeletalMeshComponent = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMeshComponent != nullptr)
    {
        auto& meshInstance = m_skeletalMeshRenderInstances.TryEmplace(pSkeletalMeshComponent->GetMesh()->GetID(), pSkeletalMeshComponent->GetMesh());
        meshInstance.m_components.PushBack(pSkeletalMeshComponent);
        return;
    }

    auto pLightComponent = dynamic_cast<LightComponent*>(pComponent);
    if (pLightComponent != nullptr)
    {
        m_renderData.m_lightComponents.PushBack(pLightComponent);
        return;
    }
}

void WorldRenderingSystem::UnregisterComponent(const Entity* pEntity, IComponent* pComponent)
{
    auto pCamera = dynamic_cast<CameraComponent*>(pComponent);
    if (pCamera != nullptr)
    {
        assert(m_renderData.m_pCameraComponent != nullptr);
        m_renderData.m_pCameraComponent = nullptr;
        return;
    }

    auto pStaticMeshComponent = dynamic_cast<StaticMeshComponent*>(pComponent);
    if (pStaticMeshComponent != nullptr)
    {
        auto& meshInstance = m_staticMeshRenderInstances.Get(pStaticMeshComponent->GetMesh()->GetID());
        meshInstance.m_components.Erase(pStaticMeshComponent);
        if (meshInstance.m_components.Empty())
        {
            m_staticMeshRenderInstances.Erase(meshInstance);
        }
        return;
    }

    auto pSkeletalMeshComponent = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMeshComponent != nullptr)
    {
        auto& meshInstance = m_skeletalMeshRenderInstances.Get(pSkeletalMeshComponent->GetMesh()->GetID());
        meshInstance.m_components.Erase(pSkeletalMeshComponent);
        if (meshInstance.m_components.Empty())
        {
            m_skeletalMeshRenderInstances.Erase(meshInstance);
        }
        return;
    }

    auto pLightComponent = dynamic_cast<LightComponent*>(pComponent);
    if (pLightComponent != nullptr)
    {
        m_renderData.m_lightComponents.Erase(pLightComponent);
        return;
    }
}

const UpdatePriorities& WorldRenderingSystem::GetUpdatePriorities()
{
    // TODO
    UpdatePriorities up;
    return up;
};

const WorldRenderingSystem::GPUResources& WorldRenderingSystem::GetGPUResources() const
{
    return m_gpuResources[m_pRenderingService->GetRenderEngine()->GetCurrentFrameIdx()];
}

} // namespace aln