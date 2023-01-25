#include "world_systems/render_system.hpp"

#include <graphics/rendering/renderer.hpp>

#include "components/camera.hpp"
#include "components/light.hpp"

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
    // Lights resources
    m_lightsBuffer = vkg::resources::Buffer(m_pRenderer->GetDevice(), 16 + (5 * sizeof(LightUniform)), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    CreateLightsDescriptorSet();

    // Per-view resources
    m_cameraUBO = vkg::resources::Buffer(m_pRenderer->GetDevice(), sizeof(vkg::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // Debug resources
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
    int nLights = m_lightComponents.Size();
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
    for (auto& meshInstance : m_StaticMeshRenderInstances)
    {
        for (const auto& pStaticMesh : meshInstance.m_components)
        {
            // Compute this mesh's model matrix
            ubo.model = pStaticMesh->GetWorldTransform().ToMatrix();

            // Update the ubo
            // pStaticMesh->UpdateUniformBuffers(ubo);
            // TODO: We should only map once and unmap right before deletion
            // TODO: For now we still copy the whole UBO for each object. Camera data should be split up and copied only once
            m_cameraUBO.Map(0, sizeof(ubo));
            m_cameraUBO.Copy(&ubo, sizeof(ubo));
            m_cameraUBO.Unmap();

            // Bind the mesh buffers
            staticMeshesPipeline.BindDescriptorSet(cb, meshInstance.m_descriptorSet.get(), 1);
            vk::DeviceSize offset = 0;
            pStaticMesh->GetMesh()->Bind(cb, offset);
        }
    }

    m_pRenderer->GetSkeletalMeshesPipeline().Bind(cb);
    for (auto& meshInstance : m_SkeletalMeshRenderInstances)
    {
        for (auto pSkeletalMeshComponent : meshInstance.m_components)
        {
            pSkeletalMeshComponent->UpdateSkinningTransforms();

            // Upload the new data to gpu
            // TODO: the buffer is host visible for now, which is not optimal
            // TODO: Only map once
            meshInstance.m_skinningBuffer.Map(0, sizeof(glm::mat4x4) * pSkeletalMeshComponent->m_skinningTransforms.size());
            meshInstance.m_skinningBuffer.Copy(pSkeletalMeshComponent->m_skinningTransforms);
            meshInstance.m_skinningBuffer.Unmap();

            ubo.model = pSkeletalMeshComponent->GetWorldTransform().ToMatrix();
            // TODO: We should only map once and unmap right before deletion
            // TODO: For now we still copy the whole UBO for each object. Camera data should be split up and copied only once
            m_cameraUBO.Map(0, sizeof(ubo));
            m_cameraUBO.Copy(&ubo, sizeof(ubo));
            m_cameraUBO.Unmap();

            m_pRenderer->GetSkeletalMeshesPipeline().BindDescriptorSet(cb, meshInstance.m_descriptorSet.get(), 1);
            pSkeletalMeshComponent->GetMesh()->Bind(cb, 0);
        }
    }

    // Debug drawing
    DrawingContext drawingContext;
    for (auto& meshInstance : m_SkeletalMeshRenderInstances)
    {
        for (const auto& pSkeletalMeshComponent : meshInstance.m_components)
        {
            if (pSkeletalMeshComponent->m_drawDebugSkeleton)
            {
                pSkeletalMeshComponent->DrawPose(drawingContext);
            }
            // pSkeletalMeshComponent->DrawBindPose(drawingContext);
        }
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

    auto pStaticMeshComponent = dynamic_cast<StaticMeshComponent*>(pComponent);
    if (pStaticMeshComponent != nullptr)
    {
        auto& meshInstance = m_StaticMeshRenderInstances.TryEmplace(pStaticMeshComponent->GetMesh()->GetID(), m_pRenderer->GetDevice(), pStaticMeshComponent->GetMesh(), &m_cameraUBO);
        meshInstance.m_components.PushBack(pStaticMeshComponent);
        return;
    }

    auto pSkeletalMeshComponent = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMeshComponent != nullptr)
    {
        auto& meshInstance = m_SkeletalMeshRenderInstances.TryEmplace(pSkeletalMeshComponent->GetMesh()->GetID(), m_pRenderer->GetDevice(), pSkeletalMeshComponent->GetMesh(), &m_cameraUBO);
        meshInstance.m_components.PushBack(pSkeletalMeshComponent);
        return;
    }

    auto pLight = dynamic_cast<Light*>(pComponent);
    if (pLight != nullptr)
    {
        m_lightComponents.PushBack(pLight);
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

    auto pStaticMeshComponent = dynamic_cast<StaticMeshComponent*>(pComponent);
    if (pStaticMeshComponent != nullptr)
    {
        auto& meshInstance = m_StaticMeshRenderInstances.Get(pStaticMeshComponent->GetMesh()->GetID());
        meshInstance.m_components.Erase(pStaticMeshComponent);
        if (meshInstance.m_components.Empty())
        {
            m_StaticMeshRenderInstances.Erase(meshInstance);
        }
        return;
    }

    auto pSkeletalMeshComponent = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMeshComponent != nullptr)
    {
        auto& meshInstance = m_SkeletalMeshRenderInstances.Get(pStaticMeshComponent->GetMesh()->GetID());
        meshInstance.m_components.Erase(pSkeletalMeshComponent);
        if (meshInstance.m_components.Empty())
        {
            m_SkeletalMeshRenderInstances.Erase(meshInstance);
        }
        return;
    }

    auto pLight = dynamic_cast<Light*>(pComponent);
    if (pLight != nullptr)
    {
        m_lightComponents.Erase(pLight);
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

GraphicsSystem::StaticMeshRenderInstance::StaticMeshRenderInstance(vkg::Device* pDevice, const StaticMesh* pMesh, vkg::resources::Buffer* pUniformBuffer) : m_pMesh(pMesh)
{
    assert(pDevice != nullptr && pMesh != nullptr);

    m_descriptorSet = pDevice->AllocateDescriptorSet<StaticMeshComponent>();
    pDevice->SetDebugUtilsObjectName(m_descriptorSet.get(), "StaticMeshComponent Descriptor Set");

    std::array<vk::WriteDescriptorSet, 3> writeDescriptors = {};

    writeDescriptors[0].dstSet = m_descriptorSet.get();
    writeDescriptors[0].dstBinding = 0;
    writeDescriptors[0].dstArrayElement = 0;
    writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptors[0].descriptorCount = 1;
    auto uniformDescriptor = pUniformBuffer->GetDescriptor();
    writeDescriptors[0].pBufferInfo = &uniformDescriptor;

    writeDescriptors[1].dstSet = m_descriptorSet.get();
    writeDescriptors[1].dstBinding = 1;
    writeDescriptors[1].dstArrayElement = 0;
    writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    writeDescriptors[1].descriptorCount = 1;
    auto textureDescriptor = m_pMesh->GetMaterial()->GetAlbedoMap()->GetDescriptor();
    writeDescriptors[1].pImageInfo = &textureDescriptor;

    // TODO: Materials presumably don't change so they don't need a binding
    writeDescriptors[2].dstSet = m_descriptorSet.get();
    writeDescriptors[2].dstBinding = 2;
    writeDescriptors[2].dstArrayElement = 0;
    writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptors[2].descriptorCount = 1;
    auto materialDescriptor = m_pMesh->GetMaterial()->GetBuffer().GetDescriptor();
    writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?

    pDevice->GetVkDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
}

GraphicsSystem::SkeletalMeshRenderInstance::SkeletalMeshRenderInstance(vkg::Device* pDevice, const SkeletalMesh* pMesh, vkg::resources::Buffer* pUniformBuffer) : m_pMesh(pMesh)
{
    assert(pDevice != nullptr && pMesh != nullptr);

    // TODO: The buffer should be device-local
    m_skinningBuffer = vkg::resources::Buffer(
        pDevice,
        m_pMesh->GetBoneCount() * sizeof(glm::mat4x4),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // Create the descriptor set
    m_descriptorSet = pDevice->AllocateDescriptorSet<SkeletalMeshComponent>();
    pDevice->SetDebugUtilsObjectName(m_descriptorSet.get(), "SkeletalMeshComponent Descriptor Set");

    std::array<vk::WriteDescriptorSet, 4> writeDescriptors = {};

    writeDescriptors[0].dstSet = m_descriptorSet.get();
    writeDescriptors[0].dstBinding = 0;
    writeDescriptors[0].dstArrayElement = 0;
    writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptors[0].descriptorCount = 1;
    auto uniformDescriptor = pUniformBuffer->GetDescriptor();
    writeDescriptors[0].pBufferInfo = &uniformDescriptor;

    writeDescriptors[1].dstSet = m_descriptorSet.get();
    writeDescriptors[1].dstBinding = 1;
    writeDescriptors[1].dstArrayElement = 0;
    writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    writeDescriptors[1].descriptorCount = 1;
    auto textureDescriptor = m_pMesh->GetMaterial()->GetAlbedoMap()->GetDescriptor();
    writeDescriptors[1].pImageInfo = &textureDescriptor;

    // TODO: Materials presumably don't change so they don't need a binding
    writeDescriptors[2].dstSet = m_descriptorSet.get();
    writeDescriptors[2].dstBinding = 2;
    writeDescriptors[2].dstArrayElement = 0;
    writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
    writeDescriptors[2].descriptorCount = 1;
    auto materialDescriptor = m_pMesh->GetMaterial()->GetBuffer().GetDescriptor();
    writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?

    writeDescriptors[3].dstSet = m_descriptorSet.get();
    writeDescriptors[3].dstBinding = 3;
    writeDescriptors[3].dstArrayElement = 0;
    writeDescriptors[3].descriptorType = vk::DescriptorType::eStorageBuffer;
    writeDescriptors[3].descriptorCount = 1;
    auto skinningBufferDescriptor = m_skinningBuffer.GetDescriptor();
    writeDescriptors[3].pBufferInfo = &skinningBufferDescriptor; // TODO: Replace w/ push constants ?

    pDevice->GetVkDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
}
} // namespace aln