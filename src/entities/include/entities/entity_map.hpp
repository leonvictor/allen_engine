#pragma once

#include "entity.hpp"
#include "entity_collection.hpp"
#include "loading_context.hpp"
#include "object_model.hpp"

#include <list>
#include <mutex>
#include <set>
#include <vector>

namespace aln::entities
{
class Entity;

class EntityMap
{
    friend class WorldEntity;

    enum class Status
    {
        Deactivated,     // Not activated
        EntitiesLoading, // Entities loading
        Loaded,          // All entities loaded
        Activated        // All entities activated. Some might still be loading in case of dynamic adds
    };

    /// Hierarchy of entities, used to display in the editor.
    /// TODO: Maybe disable outside of the editor ?
    /// @todo: fuse with another tree for culling etc ?
    std::vector<Entity*> m_entitiesTree;

    /// @todo: Entities live on the heap. Profile !
    std::vector<Entity*> m_entities;

    std::vector<Entity*> m_entitiesToAdd;
    std::vector<Entity*> m_entitiesToRemove;
    std::vector<Entity*> m_loadingEntities;
    std::vector<Entity*> m_entitiesToReload; // TODO
    std::vector<Entity*> m_entitiesToActivate;
    std::vector<Entity*> m_entitiesToDeactivate;

    std::recursive_mutex m_mutex;
    // ...
    Status m_status = Status::Deactivated;
    bool m_isTransientMap = false;

    EntityMap() {}

    /// @brief Clear this map. If it is the main one, deactivate and unload
    /// all entities in the underlying collection.
    void Clear(const LoadingContext& loadingContext);

    bool IsActivated() { return m_status == Status::Activated; }

    void ActivateEntity(Entity* pEntity);
    void DeactivateEntity(Entity* pEntity);

    // -------------------------------------------------
    // Map state management
    // -------------------------------------------------

    /// @brief Update the loading state of each entity
    /// @note This function should be called exactly once per frame.
    bool Load(const LoadingContext& loadingContext);

    /// @brief Update the systems of each entity.
    void Update(const UpdateContext& updateContext);

    /// @brief Activate all entities in the collection.
    void Activate(const LoadingContext& loadingContext);

  public:
    EntityMap(bool isTransient) : m_isTransientMap(isTransient) {}

    /// @brief Create an entity. The entity will be added to the world during the next loading phase.
    Entity* CreateEntity(std::string name = "");

    /// @brief Permanently remove an entity from the collection.
    void RemoveEntity(Entity* pEntity);
};
} // namespace aln::entities