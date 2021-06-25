#pragma once

#include "entity_collection.hpp"
#include "object_model.hpp"

#include <map>
#include <set>
#include <vector>

class Entity;

class EntityMap : private EntityCollection
{
    friend class WorldEntity;

    enum Status
    {
        EntitiesLoading,
        Loaded,
        Activated
    };

    // std::vector<Entity*> m_entitiesToAdd;
    std::vector<Entity*> m_entitiesToRemove;
    std::vector<Entity*> m_loadingEntities;
    std::vector<Entity*> m_entitiesToReload; // TODO
    // std::vector<Entity*> m_entitiesToActivate;
    // std::vector<Entity*> m_entitiesToDeactivate;

    // ...
    Status m_status;
    bool m_isTransientMap;

    bool IsActivated()
    {
        return m_status == Status::Activated;
    }

    /// @brief Remove an entity from the world.
    void RemoveEntity(Entity* pEntity);

    /// @brief Update the state of each entity
    /// @note This function should be called exactly once per frame.
    bool Update(const ObjectModel::LoadingContext& loadingContext);
};