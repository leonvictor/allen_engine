#include "world_systems/render_system.hpp"

#include <graphics/rendering/renderer.hpp>

#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh_component.hpp"
#include "components/skeletal_mesh_component.hpp"
#include "components/static_mesh_component.hpp"

#include <entities/entity.hpp>
#include <entities/update_context.hpp>
#include <entities/world_system.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

#include <Tracy.hpp>
#include <map>

namespace aln
{

void GraphicsSystem::CreateLightsDescriptorSet()
{
    m_lightsVkDescriptorSet = m_pRenderer->GetDevice()->AllocateDescriptorSet<Light>();
    m_pRenderer->GetDevice()->SetDebugUtilsObjectName(m_lightsVkDescriptorSet.get(), "Lights Descriptor Set");

    vk::DescriptorBufferInfo lightsBufferInfo;
    lightsBufferInfo.buffer = m_lightsBuffer.GetVkBuffer(); // TODO: How do we update the lights array ?
    lightsBufferInfo.offset = 0;
    lightsBufferInfo.range = VK_WHOLE_SIZE;

    vk::WriteDescriptorSet writeDescriptor;
    writeDescriptor.dstSet = m_lightsVkDescriptorSet.get();
    writeDescriptor.dstBinding = 0;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorType = vk::DescriptorType::eStorageBuffer;
    writeDescriptor.descriptorCount = 1;
    writeDescriptor.pBufferInfo = &lightsBufferInfo;

    m_pRenderer->GetDevice()->GetVkDevice().updateDescriptorSets(1, &writeDescriptor, 0, nullptr);
}

void GraphicsSystem::RenderDebugLines(vk::CommandBuffer& cb, DrawingContext& drawingContext)
{
    const auto& vertexBuffer = drawingContext.m_vertices;
    auto vertexCount = vertexBuffer.size();

    // todo: Aggregate line data coming from multiple threads

    // TODO: Profile with a staging buffer
    // but vk buffer copy is not allowed inside a render pass
    // vkg::resources::Buffer stagingBuffer(m_pRenderer->GetDevice(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexBuffer);
    // m_linesRenderState.m_stagingBuffer.Map();
    // m_linesRenderState.m_stagingBuffer.Copy(vertexBuffer);
    // m_linesRenderState.m_stagingBuffer.Unmap();

    // m_linesRenderState.m_stagingBuffer.CopyTo(cb, m_linesRenderState.m_vertexBuffer);

    // Update UBO
    LinesRenderState::UBO ubo;
    ubo.m_viewProjectionMatrix = m_pCameraComponent->GetViewProjectionMatrix(m_aspectRatio);
    m_linesRenderState.m_viewProjectionUBO.Copy(&ubo, sizeof(ubo));

    // Update vertex buffer
    m_linesRenderState.m_vertexBuffer.Copy(vertexBuffer);

    // TODO: Handle a max number of lines per draw call
    // Split the buffer accordingly and draw each part sequentially

    auto& pipeline = m_linesRenderState.m_pipeline;
    pipeline.Bind(cb);

    vk::DeviceSize offset = 0;
    pipeline.BindDescriptorSet(cb, m_linesRenderState.m_descriptorSet.get(), 0);
    cb.bindVertexBuffers(0, m_linesRenderState.m_vertexBuffer.GetVkBuffer(), offset);
    cb.draw(vertexBuffer.size(), 1, 0, 0);
}

void GraphicsSystem::Shutdown()
{
    m_lightsVkDescriptorSet.reset();
    m_lightsBuffer = vkg::resources::Buffer();
}

void GraphicsSystem::Initialize()
{
    m_lightsBuffer = vkg::resources::Buffer(m_pRenderer->GetDevice(), 16 + (5 * sizeof(LightUniform)), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    CreateLightsDescriptorSet();

    m_linesRenderState.Initialize(m_pRenderer->GetDevice(), m_pRenderer);
}

void GraphicsSystem::Update(const UpdateContext& context)
{
    ZoneScoped;

    if (context.GetUpdateStage() != UpdateStage::FrameEnd)
        return;

    assert(m_pCameraComponent != nullptr);
    if (!m_pCameraComponent->IsInitialized())
        return; // Camera is the only necessary component, return if it's not loaded yet

    // Update viewport info
    m_aspectRatio = context.GetDisplayWidth() / context.GetDisplayHeight();

    aln::vkg::render::RenderContext ctx =
        {.backgroundColor = m_pCameraComponent->m_backgroundColor};
    m_pRenderer->BeginFrame(ctx);

    // Get the active command buffer
    auto& cb = m_pRenderer->GetActiveRenderTarget().commandBuffer.get();
    auto& staticMeshesPipeline = m_pRenderer->GetStaticMeshesPipeline();

    // Bind the pipeline
    staticMeshesPipeline.Bind(cb);

    // Update the lights buffer
    int nLights = m_lightComponents.GetRegisterComponentsCount();
    m_lightsBuffer.Map(0, sizeof(int)); // TODO: Respect spec alignment for int
    m_lightsBuffer.Copy(&nLights, sizeof(int));
    m_lightsBuffer.Unmap();

    int i = 0;
    for (auto pLight : m_lightComponents)
    {
        auto ubo = pLight->GetUniform();
        m_lightsBuffer.Map(sizeof(int) + i * sizeof(LightUniform), sizeof(LightUniform));
        m_lightsBuffer.Copy(&ubo, sizeof(LightUniform));
        m_lightsBuffer.Unmap();
    }

    staticMeshesPipeline.BindDescriptorSet(cb, m_lightsVkDescriptorSet.get(), 0);

    vkg::UniformBufferObject ubo;
    Transform t = m_pCameraComponent->GetWorldTransform();
    ubo.cameraPos = t.GetTranslation();
    ubo.view = m_pCameraComponent->GetViewMatrix();
    ubo.projection = glm::perspective(
        glm::radians(m_pCameraComponent->fov),
        m_aspectRatio,
        m_pCameraComponent->nearPlane,
        m_pCameraComponent->farPlane);

    // GLM is designed for OpenGL which uses inverted y coordinates
    ubo.projection[1][1] *= -1;

    // Loop over the registered static meshes
    for (const auto& pStaticMesh : m_staticMeshComponents)
    {
        // Compute this mesh's model matrix
        ubo.model = pStaticMesh->GetWorldTransform().ToMatrix();

        // Update the ubo
        pStaticMesh->UpdateUniformBuffers(ubo);

        // Bind the mesh buffers
        staticMeshesPipeline.BindDescriptorSet(cb, pStaticMesh->GetDescriptorSet(), 1);
        vk::DeviceSize offset = 0;
        pStaticMesh->GetMesh()->Bind(cb, offset);
    }

    m_pRenderer->GetSkeletalMeshesPipeline().Bind(cb);
    for (auto pSkeletalMesh : m_skeletalMeshComponents)
    {
        pSkeletalMesh->UpdateSkinningBuffer();

        ubo.model = pSkeletalMesh->GetWorldTransform().ToMatrix();
        pSkeletalMesh->UpdateUniformBuffers(ubo);

        m_pRenderer->GetSkeletalMeshesPipeline().BindDescriptorSet(cb, pSkeletalMesh->GetDescriptorSet(), 1);
        pSkeletalMesh->GetMesh()->Bind(cb, 0);
    }

    // Debug drawing
    DrawingContext drawingContext;
    for (const auto& pSkeletalMeshComponent : m_skeletalMeshComponents)
    {
        if (pSkeletalMeshComponent->m_drawDebugSkeleton)
        {
            pSkeletalMeshComponent->DrawPose(drawingContext);
        }
        // pSkeletalMeshComponent->DrawBindPose(drawingContext);
    }

    RenderDebugLines(cb, drawingContext);

    m_pRenderer->EndFrame();
}

void GraphicsSystem::RegisterComponent(const Entity* pEntity, IComponent* pComponent)
{
    auto pCamera = dynamic_cast<Camera*>(pComponent);
    if (pCamera != nullptr)
    {
        if (m_pCameraComponent != nullptr)
        {
            std::runtime_error("The render system already has a camera registered.");
        }

        m_pCameraComponent = pCamera;
        return;
    }

    auto pStaticMesh = dynamic_cast<StaticMeshComponent*>(pComponent);
    if (pStaticMesh != nullptr)
    {
        m_staticMeshComponents.AddRecordEntry(pEntity->GetID(), pStaticMesh);
        return;
    }

    auto pSkeletalMesh = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMesh != nullptr)
    {
        m_skeletalMeshComponents.AddRecordEntry(pEntity->GetID(), pSkeletalMesh);
        return;
    }

    auto pLight = dynamic_cast<Light*>(pComponent);
    if (pLight != nullptr)
    {
        m_lightComponents.AddRecordEntry(pEntity->GetID(), pLight);
        return;
    }
}

void GraphicsSystem::UnregisterComponent(const Entity* pEntity, IComponent* pComponent)
{
    auto pCamera = dynamic_cast<Camera*>(pComponent);
    if (pCamera != nullptr)
    {
        if (m_pCameraComponent == nullptr)
        {
            std::runtime_error("Tried to remove a non-existing camera component");
        }

        m_pCameraComponent = nullptr;
        return;
    }

    auto pStaticMesh = dynamic_cast<StaticMeshComponent*>(pComponent);
    if (pStaticMesh != nullptr)
    {
        m_staticMeshComponents.RemoveRecordEntry(pEntity->GetID(), pStaticMesh);
        return;
    }

    auto pSkeletalMesh = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMesh != nullptr)
    {
        m_skeletalMeshComponents.RemoveRecordEntry(pEntity->GetID(), pSkeletalMesh);
        return;
    }

    auto pLight = dynamic_cast<Light*>(pComponent);
    if (pLight != nullptr)
    {
        m_lightComponents.RemoveRecordEntry(pEntity->GetID(), pLight);
        return;
    }
}

const UpdatePriorities& GraphicsSystem::GetUpdatePriorities()
{
    // TODO
    UpdatePriorities up;
    return up;
};

GraphicsSystem::GraphicsSystem(vkg::render::IRenderer* pRenderer)
{
    m_pRenderer = pRenderer;
}
} // namespace aln