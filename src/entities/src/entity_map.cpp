#include "entity_map.hpp"

#include "entity.hpp"

#include "loading_context.hpp"
#include "object_model.hpp"

#include <functional>
#include <stdexcept>

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
        for (auto& [id, entity] : EntityCollection::Collection())
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

bool EntityMap::Load(const LoadingContext& loadingContext)
{
    auto& newlyCreated = EntityCollection::NewlyCreatedEntities();
    // Gather up newly created entities, add them to the loading list and move them to the static collection
    for (auto it = newlyCreated.begin(); it != newlyCreated.end();)
    {
        // TODO: this is wonky
        auto [nit, value] = EntityCollection::Collection().insert(std::move(*it));
        m_loadingEntities.push_back(&(nit->second));
        it = newlyCreated.erase(it);
        // TODO: Actually probably all the lists in the map should be in all threads and synced for the update
        // (cause all threads could ask for an entity removal, or even loading)
    }
    newlyCreated.clear();

    if (m_loadingEntities.size() > 0 && m_status == Status::Deactivated)
    {
        m_status = Status::EntitiesLoading;
    }

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
            // With eastl:
            // m_loadingEntities.erase_first_unsorted(pEntityToRemove->m_ID);
            // m_entitiesToReload.erase_first_unsorted(pEntityToRemove->m_ID);

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

    return true;
}

void EntityMap::Activate(const LoadingContext& loadingContext)
{
    auto& collection = Collection();
    for (auto it = collection.begin(); it != collection.end(); ++it)
    {
        auto& entity = it->second;
        if (entity.IsLoaded())
        {
            entity.Activate(loadingContext);
        }
    }
}
}