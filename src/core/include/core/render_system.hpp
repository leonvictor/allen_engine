#pragma once

#include <graphics/rendering/renderer.hpp>

#include <core/camera.hpp>
#include <core/light.hpp>
#include <core/mesh_renderer.hpp>

#include <object_model/entity.hpp>
#include <object_model/object_model.hpp>
#include <object_model/world_system.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

#include <map>

class GraphicsSystem : public IWorldSystem
{
    vkg::IRenderer* m_pRenderer = nullptr;

    // TODO: Replace the map with something else. We *can* have multiple meshes per entity
    std::map<const Entity*, MeshRenderer*> m_components;
    std::map<const Entity*, Light*> m_lightComponents;
    Camera* m_pCameraComponent = nullptr;

    // Lights use a shared buffer and descriptorSet, so it's held in the system
    vkg::Buffer m_lightsBuffer;
    vk::UniqueDescriptorSet m_lightsVkDescriptorSet;

    void CreateLightsDescriptorSet()
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

    void Shutdown() override
    {
        m_lightsVkDescriptorSet.reset();
        m_lightsBuffer = vkg::Buffer();
    }

    void Initialize() override
    {
        m_lightsBuffer = vkg::Buffer(m_pRenderer->GetDevice(), 16 + (5 * sizeof(LightUniform)), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        CreateLightsDescriptorSet();
    }

    void Update(const ObjectModel::UpdateContext& context) override
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
            vk::DeviceSize offset = 0;
            cb.bindVertexBuffers(0, pMeshRenderer->m_vertexBuffer.GetVkBuffer(), offset);
            cb.bindIndexBuffer(pMeshRenderer->m_indexBuffer.GetVkBuffer(), offset, vk::IndexType::eUint32);

            objectPipeline.BindDescriptorSet(cb, pMeshRenderer->GetDescriptorSet(), 1);
            cb.drawIndexed(pMeshRenderer->m_mesh.m_indices.size(), 1, 0, 0, 0);
        }

        m_pRenderer->EndFrame();
    }

    void RegisterComponent(const Entity* pEntity, IComponent* pComponent)
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

    void UnregisterComponent(const Entity* pEntity, IComponent* pComponent)
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

    const UpdatePriorities& GetUpdatePriorities() override
    {
        // TODO
        UpdatePriorities up;
        return up;
    };

  public:
    GraphicsSystem(vkg::IRenderer* pRenderer)
    {
        m_pRenderer = pRenderer;
    }
};