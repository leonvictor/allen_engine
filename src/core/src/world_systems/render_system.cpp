#include "world_systems/render_system.hpp"

#include "components/camera.hpp"
#include "components/light.hpp"
#include "renderers/scene_renderer.hpp"
#include "services/rendering_service.hpp"

#include <common/maths/matrix4x4.hpp>
#include <entities/entity.hpp>
#include <entities/update_context.hpp>
#include <entities/world_system.hpp>
#include <graphics/rendering/renderer.hpp>

#include <tracy/Tracy.hpp>

namespace aln
{

void GraphicsSystem::RenderDebugLines(vk::CommandBuffer& cb, DrawingContext& drawingContext)
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

void GraphicsSystem::Shutdown(const ServiceProvider& serviceProvider)
{
    // TODO
    //m_linesRenderState.Shutdown();
}

void GraphicsSystem::Initialize(const ServiceProvider& serviceProvider)
{
    // Debug resources
    // TODO: Rework line debugging
    //m_linesRenderState.Initialize(m_pRenderer->GetDevice(), m_pRenderer);
}

void GraphicsSystem::Update(const UpdateContext& context)
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

    // TODO: The graphics system is merely responsible for culling, the true rendering command
    // recording is done by the graphics service. 
    // How do the two communicate ?
    // Conceptually it would be better if Systems were clients of services
    // So set the things to render from here to the service
    // Problem is we might have multiple worlds that should be rendered by different renderers
    // (i.e. editor preview windows)
    // Previews can have their own world, but the service should be shared
    auto pRenderingService = context.GetService<RenderingService>();

    // Update viewport info
    m_aspectRatio = context.GetDisplayWidth() / context.GetDisplayHeight();

    //aln::RenderContext ctx = {.backgroundColor = m_pCameraComponent->m_backgroundColor};
    //m_pRenderer->StartFrame(ctx);

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

    /// @todo Keep cb inside the renderer once we've moved debug rendering
    //auto& cb = m_pRenderer->GetActiveRenderTarget().commandBuffer.get();
    //m_pRenderer->Render(data, cb);

    // Debug drawing
    /// @todo: Move to a dedicated renderer
    //DrawingContext drawingContext;
    //for (auto& meshInstance : m_skeletalMeshRenderInstances)
    //{
    //    for (const auto& pSkeletalMeshComponent : meshInstance.m_components)
    //    {
    //        if (pSkeletalMeshComponent->m_drawDebugSkeleton)
    //        {
    //            pSkeletalMeshComponent->DrawPose(drawingContext);
    //        }
    //        // pSkeletalMeshComponent->DrawBindPose(drawingContext);
    //    }
    //}

    //RenderDebugLines(cb, drawingContext);

    //m_pRenderer->EndFrame();
}

void GraphicsSystem::RegisterComponent(const Entity* pEntity, IComponent* pComponent)
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

    auto pLight = dynamic_cast<Light*>(pComponent);
    if (pLight != nullptr)
    {
        m_renderData.m_lightComponents.PushBack(pLight);
        return;
    }
}

void GraphicsSystem::UnregisterComponent(const Entity* pEntity, IComponent* pComponent)
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

    auto pLight = dynamic_cast<Light*>(pComponent);
    if (pLight != nullptr)
    {
        m_renderData.m_lightComponents.Erase(pLight);
        return;
    }
}

const UpdatePriorities& GraphicsSystem::GetUpdatePriorities()
{
    // TODO
    UpdatePriorities up;
    return up;
};

} // namespace aln