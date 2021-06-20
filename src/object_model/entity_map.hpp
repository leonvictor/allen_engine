#include "entity.hpp"
#include "object_model.cpp"
#include <stdexcept>

/// @brief Singleton holder of all the entities.
// TODO: Should be responsible for ensuring concurrency
// TODO: maybe use shared_ptr ? And make sure this is the last place an entity is destroyed.
class EntityCollection
{
  private:
    static std::map<core::UUID, Entity*> m_collection;

  public:
    static void AddEntity(Entity* pEntity)
    {
        EntityCollection::m_collection.insert(std::make_pair(pEntity->m_ID, pEntity));
    }

    static void RemoveEntity(const core::UUID& id)
    {
        EntityCollection::m_collection.erase(id);
    }
};

class EntityMap
{
    enum Status
    {
        EntitiesLoading,
        Loaded,
        Activated
    };

    std::vector<Entity*> m_entitiesToRemove;
    std::vector<Entity*> m_loadingEntities;
    std::vector<Entity*> m_entitiesToReload;
    // ...
    Status m_status;
    bool m_isTransientMap;

    bool IsActivated()
    {
        return m_status == Status::Activated;
    }

    bool Update()
    {
        // ...
        ObjectModel::LoadingContext loadingContext;
        // ...

        // Unload and deactivate entities and remove them from the collection
        for (auto pEntityToRemove : m_entitiesToRemove)
        {
            // Deactivate if activated
            if (pEntityToRemove->IsActivated())
            {
                pEntityToRemove->Deactivate(loadingContext);
            }
            else // Remove from loading list as we might still be loading this entity
            {
                // TODO: Removing from vectors is meh. Profile and check if std::list/std::multimap are more efficient for what we do
                auto itLoading = std::find(m_loadingEntities.begin(), m_loadingEntities.end(), [pEntityToRemove](auto pEntity)
                                           { pEntity->m_ID == pEntityToRemove->m_ID });
                if (itLoading != m_loadingEntities.end())
                {
                    m_loadingEntities.erase(itLoading);
                }

                // m_loadingEntities.erase_first_unsorted(pEntityToRemove->m_ID);
                // m_entitiesToReload.erase_first_unsorted(pEntityToRemove->m_ID);

                auto itReload = std::find(m_entitiesToReload.begin(), m_entitiesToReload.end(), [pEntityToRemove](auto pEntity)
                                          { pEntity->m_ID == pEntityToRemove->m_ID });
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
        for (size_t i = m_loadingEntities.size() - 1; i >= 0; i--)
        {
            auto pEntity = m_loadingEntities[i];
            if (pEntity->UpdateLoadingAndEntityState(loadingContext))
            {
                // Removed loaded entity from loading list
                m_loadingEntities.erase(m_loadingEntities.begin() + i);
                // VectorEraseUnsorted(m_loadingEntities, i);

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

        // Ensure that we set the status to loaded, if we were in the entity loading stage and all ...
        // TODO:
        if (m_status == Status::EntitiesLoading && m_loadingEntities.empty())
        {
            assert(!m_isTransientMap);
            m_status = Status::Loaded;
        }
    }
};