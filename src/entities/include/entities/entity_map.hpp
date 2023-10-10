#pragma once

#include "component.hpp"
#include "entity.hpp"
#include "loading_context.hpp"
#include "update_context.hpp"

#include <common/containers/vector.hpp>
#include <common/containers/hash_map.hpp>

#include <mutex>

namespace aln
{

class EntityMap
{
    friend class WorldEntity;
    friend class EntityMapDescriptor;

    enum class Status
    {
        Unloaded, // Not activated
        Loaded,   // All entities loaded
        Activated // All entities activated. Some might still be loading in case of dynamic adds
    };

    /// @todo: Entities live on the heap. Profile !
    Vector<Entity*> m_entities;
    HashMap<UUID, Entity*> m_entityLookupMap;

    Vector<Entity*> m_entitiesToAdd;
    Vector<Entity*> m_entitiesToRemove;
    Vector<Entity*> m_loadingEntities;
    Vector<Entity*> m_editedEntities;
    Vector<Entity*> m_entitiesToActivate;
    Vector<Entity*> m_entitiesToDeactivate;

    UUID m_entityUpdateEventListenerID;

    // Mutex guarding the main entity collection
    std::recursive_mutex m_mutex;

    Status m_status = Status::Unloaded;
    bool m_isTransientMap = false;

    /// @brief Clear this map. If it is the main one, deactivate and unload
    /// all entities in the underlying collection.
    void Clear(const LoadingContext& loadingContext);

    inline bool IsActivated() const { return m_status == Status::Activated; }
    inline bool IsLoaded() const { return m_status == Status::Loaded; }
    inline bool IsUnloaded() const { return m_status == Status::Unloaded; }

    void ActivateEntity(Entity* pEntity);
    void DeactivateEntity(Entity* pEntity);

    // -------------------------------------------------
    // Map state management
    // -------------------------------------------------

    /// @brief Load the initial state of the map itself, along with already contained entities
    void Load(const LoadingContext& loadingContext);
    void Unload(const LoadingContext& loadingContext);

    /// @brief Activate the map and all associated entities
    /// @todo split activation/loading contexts
    void Activate(const LoadingContext& loadingContext);
    void Deactivate(const LoadingContext& loadingContext);

    /// @brief Update the map's state, and process the state change requested for all entities
    void UpdateEntitiesState(const LoadingContext& loadingContext);

    // --------- Event Handling

    void OnEntityStateChanged(Entity* pEntity)
    {
        // TODO: Mutex guard ?
        m_loadingEntities.push_back(pEntity);
    }

  public:
    EntityMap(bool isTransient = false);
    
    ~EntityMap();
    EntityMap(EntityMap& other) = delete;
    EntityMap(EntityMap&& other) = delete;
    void operator=(EntityMap& other) = delete;

    /// @brief Create an entity. The entity will be added to the world during the next loading phase.
    Entity* CreateEntity(std::string name = "");

    /// @brief Permanently remove an entity from the collection.
    void RemoveEntity(Entity* pEntity);

    /// @brief Find an entity by ID.
    Entity* FindEntity(const UUID& entityID) const
    {
        auto it = m_entityLookupMap.find(entityID);
        assert(it != m_entityLookupMap.end());
        return it->second;
    }

    // -------- Editing
    // TODO: Disable in prod

    void StartComponentEditing(const LoadingContext& loadingContext, IComponent* pComponent)
    {
        assert(pComponent != nullptr);
        auto pEntity = FindEntity(pComponent->GetEntityID());

        pEntity->StartComponentEditing(loadingContext, pComponent);

        if (std::find(m_editedEntities.begin(), m_editedEntities.end(), pEntity) == m_editedEntities.end())
        {
            m_editedEntities.push_back(pEntity);
        }
    }
};
} // namespace aln