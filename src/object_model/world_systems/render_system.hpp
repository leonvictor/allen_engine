#pragma once

#include "../../graphics/rendering/renderer.hpp"
#include "../components/mesh_renderer.hpp"
#include "../world_system.hpp"

#include <map>

class GraphicSystem : public IWorldSystem
{
    IRenderer* m_pRenderer;
    std::map<Entity*, MeshRenderer*> m_components;
    Camera* m_pCamera;

    // TODO: We need two components: texture and mesh
    void Shutdown() override
    {
    }

    void Update(const ObjectModel::UpdateContext& context) override
    {
        // Loop over the registered components
        for (auto& [pEntity, pMeshRenderer] : m_components)
        {
            vk::DeviceSize offset = 0;

            cb.bindVertexBuffers(0, pMeshRenderer.m_vertexBuffer.GetVkBuffer(), offset);
            cb.bindIndexBuffer(pMeshRenderer.m_indexBuffer.GetVkBuffer(), offset, vk::IndexType::eUint32);
        }

        // Draw each

        auto cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();
        // Objects
        pipelines.objects.Bind(cb);
        pipelines.objects.BindDescriptorSet(cb, lightsDescriptorSet.get(), 0);
        for (auto model : objects)
        {
            // TODO: Decouple this.
            auto mesh = model->getComponent<Mesh>();

            mesh->Bind(cb);
            pipelines.objects.BindDescriptorSet(cb, model->GetDescriptorSet(), 1);
            cb.drawIndexed(mesh->indices.size(), 1, 0, 0, 0);
        }
    }

    void RegisterComponent(const Entity* pEntity, IComponent* pComponent)
    {
        auto pCamera = dynamic_cast<Camera>(pComponent);
        if (pCamera != nullptr)
        {
            if (m_pCamera != nullptr)
            {
                std::runtime_error("The render system already has a camera registered.");
            }
            m_pCamera = pCamera;
        }

        auto pMeshRenderer = dynamic_cast<MeshRenderer*>(pComponent);
        if (pMeshRenderer != nullptr)
        {
            m_components.push_back(std::make_pair(pEntity, pMeshRenderer));
        }
    }

    void UnregisterComponent(const Entity* pEntity, IComponent* pComponent)
    {
        // TODO: Maps are not ideal
        m_components.erase(pEntity);
    }
};