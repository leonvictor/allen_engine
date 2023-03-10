#include "entity_map.hpp"

#include "entity.hpp"
#include "loading_context.hpp"
#include "update_context.hpp"

#include <common/threading/task_service.hpp>
#include <reflection/type_info.hpp>

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

namespace aln
{

EntityMap::EntityMap(bool isTransient)
    : m_isTransientMap(isTransient),
      m_entityUpdateEventListenerID(Entity::EntityStateUpdatedEvent.BindListener([this](Entity* pEntity)
          { OnEntityStateChanged(pEntity); })) {}

EntityMap::~EntityMap()
{
    Entity::EntityStateUpdatedEvent.UnbindListener(m_entityUpdateEventListenerID);
}

void EntityMap::Clear(const LoadingContext& loadingContext)
{
    m_entitiesToRemove.clear();
    m_loadingEntities.clear();

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

            aln::Delete(pEntity);
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
    // --------- Added entities
    for (auto pEntity : m_entitiesToAdd)
    {
        m_entities.push_back(pEntity);

        pEntity->LoadComponents(loadingContext);
        m_loadingEntities.push_back(pEntity);
    }
    m_entitiesToAdd.clear();

    // --------- Removed entities
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
            auto itLoading = std::find(m_loadingEntities.begin(), m_loadingEntities.end(), pEntityToRemove);
            if (itLoading != m_loadingEntities.end())
            {
                m_loadingEntities.erase(itLoading);
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

    // ------- Entities currently loading
    // TODO: Parallelize
    std::vector<Entity*> stillLoadingEntities;
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

    m_entitiesToAdd.push_back(pEntity);

    return pEntity;
}
} // namespace aln