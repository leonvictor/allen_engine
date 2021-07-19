#include "render_system.hpp"

#include <graphics/rendering/renderer.hpp>

#include "camera.hpp"
#include "light.hpp"
#include "mesh_renderer.hpp"

#include <entities/entity.hpp>
#include <entities/object_model.hpp>
#include <entities/world_system.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

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

void GraphicsSystem::Shutdown()
{
    m_lightsVkDescriptorSet.reset();
    m_lightsBuffer = vkg::resources::Buffer();
}

void GraphicsSystem::Initialize()
{
    m_lightsBuffer = vkg::resources::Buffer(m_pRenderer->GetDevice(), 16 + (5 * sizeof(LightUniform)), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    CreateLightsDescriptorSet();
}

void GraphicsSystem::Update(const aln::entities::UpdateContext& context)
{
    if (context.GetUpdateStage() != UpdateStage::FrameEnd)
        return;

    assert(m_pCameraComponent != nullptr);
    if (!m_pCameraComponent->IsInitialized())
        return; // Camera is the only necessary component, return if it's not loaded yet

    m_pRenderer->BeginFrame();

    // Get the active command buffer
    auto& cb = m_pRenderer->GetActiveRenderTarget().commandBuffer.get();
    auto& objectPipeline = m_pRenderer->GetObjectsPipeline();

    // Bind the pipeline
    objectPipeline.Bind(cb);

    // Update the lights buffer
    int nLights = m_lightComponents.size();
    m_lightsBuffer.Map(0, sizeof(int)); // TODO: Respect spec alignment for int
    m_lightsBuffer.Copy(&nLights, sizeof(int));
    m_lightsBuffer.Unmap();

    int i = 0;
    for (auto& [pEntity, pLight] : m_lightComponents)
    {
        auto ubo = pLight->GetUniform();
        m_lightsBuffer.Map(sizeof(int) + i * sizeof(LightUniform), sizeof(LightUniform));
        m_lightsBuffer.Copy(&ubo, sizeof(LightUniform));
        m_lightsBuffer.Unmap();
    }

    objectPipeline.BindDescriptorSet(cb, m_lightsVkDescriptorSet.get(), 0);

    float aspectRatio = context.displayWidth / (float) context.displayHeight;

    vkg::UniformBufferObject ubo;
    Transform t = m_pCameraComponent->GetWorldTransform();
    ubo.cameraPos = t.position;
    ubo.view = m_pCameraComponent->GetViewMatrix();
    ubo.projection = glm::perspective(
        glm::radians(m_pCameraComponent->fov),
        aspectRatio,
        m_pCameraComponent->nearPlane,
        m_pCameraComponent->farPlane);

    // GLM is designed for OpenGL which uses inverted y coordinates
    ubo.projection[1][1] *= -1;

    // Loop over the registered components
    for (auto& [pEntity, pMeshRenderer] : m_components)
    {
        // Compute this mesh's model matrix
        Transform transform = pMeshRenderer->GetLocalTransform();
        ubo.model = glm::mat4(1.0f);
        ubo.model = glm::translate(ubo.model, transform.position);
        ubo.model = glm::rotate(ubo.model, glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
        ubo.model = glm::rotate(ubo.model, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
        ubo.model = glm::rotate(ubo.model, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
        ubo.model = glm::scale(ubo.model, transform.scale);

        // Update the ubo
        pMeshRenderer->UpdateUniformBuffers(ubo);

        // Bind the mesh buffers
        objectPipeline.BindDescriptorSet(cb, pMeshRenderer->GetDescriptorSet(), 1);
        vk::DeviceSize offset = 0;
        pMeshRenderer->m_mesh.Bind(cb, offset);
    }

    m_pRenderer->EndFrame();
}

void GraphicsSystem::RegisterComponent(const entities::Entity* pEntity, entities::IComponent* pComponent)
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

    auto pMeshRenderer = dynamic_cast<MeshRenderer*>(pComponent);
    if (pMeshRenderer != nullptr)
    {
        m_components.emplace(std::make_pair(pEntity, pMeshRenderer));
        return;
    }

    auto pLight = dynamic_cast<Light*>(pComponent);
    if (pLight != nullptr)
    {
        m_lightComponents.emplace(std::make_pair(pEntity, pLight));
        return;
    }
}

void GraphicsSystem::UnregisterComponent(const entities::Entity* pEntity, entities::IComponent* pComponent)
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

    auto pMeshRenderer = dynamic_cast<MeshRenderer*>(pComponent);
    if (pMeshRenderer != nullptr)
    {
        // TODO: Maps are not ideal
        auto it = m_components.find(pEntity);
        assert(it != m_components.end());
        m_components.erase(it);
        return;
    }

    auto pLight = dynamic_cast<Light*>(pComponent);
    if (pLight != nullptr)
    {
        auto it = m_lightComponents.find(pEntity);
        assert(it != m_lightComponents.end());
        m_lightComponents.erase(it);
        return;
    }
}

const entities::UpdatePriorities& GraphicsSystem::GetUpdatePriorities()
{
    // TODO
    entities::UpdatePriorities up;
    return up;
};

GraphicsSystem::GraphicsSystem(vkg::render::IRenderer* pRenderer)
{
    m_pRenderer = pRenderer;
}
} // namespace aln