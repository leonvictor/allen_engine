#include "entity_map.hpp"

#include "entity.hpp"
#include "loading_context.hpp"
#include "update_context.hpp"

#include <common/threading/task_service.hpp>
#include <reflection/reflection.hpp>

#include <algorithm>
#include <array>
#include <execution>
#include <functional>
#include <mutex>
#include <semaphore>
#include <stdexcept>
#include <thread>

#include <Tracy.hpp>
#include <common/TracySystem.hpp>
#include <fmt/core.h>

namespace aln::entities
{
void EntityMap::Clear(const LoadingContext& loadingContext)
{
    m_entitiesToRemove.clear();
    m_loadingEntities.clear();
    m_entitiesToReload.clear();

    // If this is the main map, deactivate and unload all entities,
    // then clear the collection.
    if (!m_isTransientMap)
    {
        for (auto& pEntity : m_entities)
        {
            if (pEntity->IsActivated())
            {
                pEntity->Deactivate(loadingContext);
            }
            pEntity->UnloadComponents(loadingContext);
        }
        m_entities.clear();
    }
}

void EntityMap::RemoveEntity(Entity* pEntity)
{
    m_entitiesToRemove.push_back(pEntity);
}

void EntityMap::ActivateEntity(Entity* pEntity)
{
    m_entitiesToActivate.push_back(pEntity);
}

void EntityMap::DeactivateEntity(Entity* pEntity)
{
    m_entitiesToDeactivate.push_back(pEntity);
}

void EntityMap::Load(const LoadingContext& loadingContext)
{
    assert(IsUnloaded());
    // TODO: Handle deserialization of a saved map
    m_status = Status::Loaded;
}

void EntityMap::Activate(const LoadingContext& loadingContext)
{
    struct ActivationTask : public ITaskSet
    {
        const std::vector<Entity*> m_entities;
        const LoadingContext& m_loadingContext;

        ActivationTask(const std::vector<Entity*>& entities, const LoadingContext& loadingContext)
            : ITaskSet(m_entities.size()), m_entities(entities), m_loadingContext(loadingContext) {}

        virtual void ExecuteRange(TaskSetPartition range, uint32_t threadNum) final override
        {
            for (auto i = range.start; i < range.end; ++i)
            {
                const auto pEntity = m_entities[i];
                if (pEntity->IsLoaded())
                {
                    pEntity->Activate(m_loadingContext);
                }
            }
        }
    };

    assert(IsLoaded());

    // auto activationTask = ActivationTask(m_entities, loadingContext);
    // loadingContext.m_pTaskService->ExecuteTask(&activationTask);

    m_status = Status::Activated;
}

void EntityMap::UpdateEntitiesState(const LoadingContext& loadingContext)
{
    // Manage the events registered by entities outside of the loading phase
    // TODO: Where should this logic be ?
    for (auto* pEntity : Entity::EntityStateUpdatedEvent.m_updatedEntities)
    {
        // TODO: Maybe early out if the entity is in the "remove" list ?
        for (auto action : pEntity->m_deferredActions)
        {
            switch (action.m_type)
            {
            case EntityInternalStateAction::Type::AddComponent:
            {
                auto pParentComponent = pEntity->GetSpatialComponent(action.m_ID);
                pEntity->AddComponentDeferred(loadingContext, (IComponent*) action.m_ptr, pParentComponent);
                break;
            }

            case EntityInternalStateAction::Type::DestroyComponent:
            {
                pEntity->DestroyComponentDeferred(loadingContext, (IComponent*) action.m_ptr);
                break;
            }

            case EntityInternalStateAction::Type::CreateSystem:
            {
                pEntity->CreateSystemDeferred(loadingContext, (aln::reflect::TypeDescriptor*) action.m_ptr);
                break;
            }

            case EntityInternalStateAction::Type::DestroySystem:
            {
                pEntity->DestroySystemDeferred(loadingContext, (aln::reflect::TypeDescriptor*) action.m_ptr);
                break;
            }

            default:
                throw std::runtime_error("Unsupported operation");
            }
        }
        pEntity->m_deferredActions.clear();
    }
    Entity::EntityStateUpdatedEvent.m_updatedEntities.clear();

    // ------------------
    // Remove entities marked for removal
    // ------------------
    for (auto pEntityToRemove : m_entitiesToRemove)
    {
        // Deactivate
        if (pEntityToRemove->IsActivated())
        {
            pEntityToRemove->Deactivate(loadingContext);
        }
        else
        {
            // Remove from loading lists as we might still be loading this entity
            // @todo: does vector::erase work ?
            auto itLoading = std::find(m_loadingEntities.begin(), m_loadingEntities.end(), pEntityToRemove);
            if (itLoading != m_loadingEntities.end())
            {
                m_loadingEntities.erase(itLoading);
            }

            auto itReload = std::find(m_entitiesToReload.begin(), m_entitiesToReload.end(), pEntityToRemove);
            if (itReload != m_entitiesToReload.end())
            {
                m_entitiesToReload.erase(itReload);
            }
        }

        // Unload entity components
        pEntityToRemove->UnloadComponents(loadingContext);

        // Remove from collection
        auto itEntity = std::find(m_entities.begin(), m_entities.end(), pEntityToRemove);
        assert(itEntity != m_entities.end());
        m_entities.erase(itEntity);

        // Release memory
        aln::Delete(pEntityToRemove);
    }

    m_entitiesToRemove.clear();

    // Entity loading
    std::vector<Entity*> stillLoadingEntities;
    // TODO: Parallelize
    for (auto pEntity : m_loadingEntities)
    {
        if (pEntity->UpdateLoadingAndEntityState(loadingContext))
        {
            // If the map is activated, immediately activate any entities that finish loading
            if (IsActivated() && !pEntity->IsActivated())
            {
                pEntity->Activate(loadingContext);
            }
        }
        else // Entity is still loading
        {
            stillLoadingEntities.push_back(pEntity);
        }
    }
    m_loadingEntities = stillLoadingEntities;

    // TODO: All entities are activated in parallel.
    // As the map will be instanciated for multiple threads this is ok for now i think ?
    // We can call entity->Activate() for each of them because it manages the local parts of the activation,
    // the rest is deffered.
    for (auto pEntity : m_entitiesToActivate)
    {
        pEntity->Activate(loadingContext);
    }
    m_entitiesToActivate.clear();

    for (auto pEntity : m_entitiesToDeactivate)
    {
        pEntity->Deactivate(loadingContext);
    }
    m_entitiesToDeactivate.clear();
}

Entity* EntityMap::CreateEntity(std::string name)
{
    std::lock_guard lock(m_mutex);

    auto pEntity = aln::New<Entity>();
    pEntity->m_name = name;
    m_entities.push_back(pEntity);
    // TODO: What's the condition ?
    // if (IsActivated())
    m_loadingEntities.push_back(pEntity);

    return pEntity;
}
} // namespace aln::entities