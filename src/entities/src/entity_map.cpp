#include "entity_map.hpp"

#include "entity.hpp"
#include "loading_context.hpp"
#include "object_model.hpp"

#include <reflection/reflection.hpp>

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
        for (auto& entity : EntityCollection::Collection())
        {
            if (entity.IsActivated())
            {
                entity.Deactivate(loadingContext);
            }
            entity.UnloadComponents(loadingContext);
        }
        EntityCollection::Clear();
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

bool EntityMap::Load(const LoadingContext& loadingContext)
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
            case EntityInternalStateAction::Type::ParentChanged:
            {
                auto it = std::find(m_entitiesTree.begin(), m_entitiesTree.end(), pEntity);
                if (pEntity->HasParentEntity()) // It shouldn't be at the tree root
                {
                    if (it != m_entitiesTree.end())
                    {
                        m_entitiesTree.erase(it);
                    }
                }
                else
                {
                    if (it == m_entitiesTree.end()) // Add it to the tree root if it previously had a parent
                    {
                        m_entitiesTree.push_back(pEntity);
                    } // Otherwise it's already in the right place
                }
                break;
            }

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

    ///////////////
    // Sync the newly created entities
    ///////////////

    // // Gather up newly created entities, add them to the loading list and move them to the static collection
    // auto& newlyCreated = EntityCollection::NewlyCreatedEntities();
    // for (auto it = newlyCreated.begin(); it != newlyCreated.end();)
    // {
    //     // TODO: this is wonky
    //     auto [nit, value] = EntityCollection::Collection().insert(std::move(*it));
    //     m_loadingEntities.push_back(&(nit->second));
    //     it = newlyCreated.erase(it);
    //     // TODO: Actually probably all the lists in the map should be in all threads and synced for the update
    //     // (cause all threads could ask for an entity removal, or even loading)
    // }
    // newlyCreated.clear();

    // Deactivate, unload and remove entities from the collection
    for (auto pEntityToRemove : m_entitiesToRemove)
    {
        // Deactivate if activated
        if (pEntityToRemove->IsActivated())
        {
            pEntityToRemove->Deactivate(loadingContext);
        }
        else // Remove from loading lists as we might still be loading this entity
        {
            // TODO: Removing from vectors is meh. Profile and check if std::list/std::multimap are more efficient for what we do
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

        // Unload components and remove from collection
        pEntityToRemove->UnloadComponents(loadingContext);
        EntityCollection::RemoveEntity(pEntityToRemove->m_ID); // ?
    }
    m_entitiesToRemove.clear();

    // Entity loading
    if (m_loadingEntities.size() > 0 && m_status == Status::Deactivated)
    {
        m_status = Status::EntitiesLoading;
    }

    for (size_t i = m_loadingEntities.size(); i > 0; i--)
    {
        auto pEntity = m_loadingEntities[i - 1];
        if (pEntity->UpdateLoadingAndEntityState(loadingContext))
        {
            // Remove loaded entity from loading list
            m_loadingEntities.erase(m_loadingEntities.begin() + i - 1);

            // If the map is activated, immediately activate any entities that finish loading
            if (IsActivated() && !pEntity->IsActivated())
            {
                pEntity->Activate(loadingContext);

                if (!pEntity->HasParentEntity())
                {
                    m_entitiesTree.push_back(pEntity);
                }
            }
        }
        else // Entity is still loading
        {
            return false; // TODO ?
        }
    }

    // Ensure that we set the status to loaded, if we were in the entity loading stage and all entities were successfully loaded
    // TODO:
    if (m_status == Status::EntitiesLoading && m_loadingEntities.empty())
    {
        assert(!m_isTransientMap);
        m_status = Status::Loaded;
    }

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

    return true;
}

void EntityMap::Activate(const LoadingContext& loadingContext)
{
    for (auto& entity : Collection())
    {
        if (entity.IsLoaded())
        {
            entity.Activate(loadingContext);

            if (!entity.HasParentEntity())
            {
                m_entitiesTree.push_back(&entity);
            }
        }
    }
    m_status = Status::Activated;
}

struct UpdateTask
{

    typedef std::list<Entity>::iterator iter;

    iter m_begin;
    iter m_end;
    UpdateContext m_updateContext;
    int m_threadNumber;

    UpdateTask(iter begin, iter end, UpdateContext updateContext, int thread_number)
        : m_begin(begin), m_end(end), m_updateContext(updateContext), m_threadNumber(thread_number) {}

    void operator()()
    {
        // tracy::SetThreadName(fmt::format("System updates ({})", m_threadNumber).c_str());
        for (auto it = m_begin; it != m_end; it++)
        {
            // TODO: Customize the context to allow further steps to populate a thread-specific map
            // TODO: When we join, we need to populate all of the maps
            it->UpdateSystems(m_updateContext);
        }
    }
};
void EntityMap::Update(const UpdateContext& updateContext)
{
    ZoneScoped;
    const int num_threads = std::thread::hardware_concurrency();

    int grainsize = Collection().size() / num_threads;
    if (grainsize < 1)
        grainsize = 1;

    std::vector<EntityMap> transientMaps;
    transientMaps.reserve(num_threads);

    auto work_iter = std::begin(Collection());
    std::vector<UpdateTask> tasks;
    tasks.reserve(num_threads);

    for (uint8_t i = 0; i != num_threads - 1 && work_iter != Collection().end(); i++)
    {
        EntityMap& transientMap = transientMaps.emplace_back(true);
        UpdateContext threadContext = updateContext;
        threadContext.pEntityMap = &transientMap;

        auto end = std::next(work_iter, grainsize);
        tasks.push_back(UpdateTask(work_iter, end, threadContext, i));
        work_iter = end;
    }

    // The remaining systems could update in the main thread maybe ?
    EntityMap& transientMap = transientMaps.emplace_back(true);
    UpdateContext threadContext = updateContext;
    threadContext.pEntityMap = &transientMap;
    tasks.push_back(UpdateTask(work_iter, Collection().end(), threadContext, tasks.size()));

    std::for_each(std::execution::par, tasks.begin(), tasks.end(), [](auto& task)
        { task(); });

    tasks.clear();

    // Sync updated entities
    // for (auto& map : transientMaps)
    // {
    //     // Gather up newly created entities, add them to the main loading list and move them to the static collection
    //     auto& newlyCreated = map.m_createdEntities;
    //     for (auto it = newlyCreated.begin(); it != newlyCreated.end();)
    //     {
    //         // TODO: this is wonky
    //         auto [nit, value] = EntityCollection::Collection().insert(std::move(*it));
    //         m_loadingEntities.push_back(&(nit->second));
    //         // it = newlyCreated.erase(it);
    //         // TODO: Actually probably all the lists in the map should be in all threads and synced for the update
    //         // (cause all threads could ask for an entity removal, or even loading)
    //     }
    //     newlyCreated.clear();
    // }
    // transientMaps.clear();
}

Entity* EntityMap::CreateEntity(std::string name)
{
    auto& collec = m_isTransientMap ? m_createdEntities : Collection();
    Entity& entity = collec.emplace_back();
    entity.m_name = name;

    if (!m_isTransientMap)
    {
        // Entity was created in the main thread, immediately start loading it
        m_loadingEntities.push_back(&entity);
    }

    return &entity;
}
} // namespace aln::entities