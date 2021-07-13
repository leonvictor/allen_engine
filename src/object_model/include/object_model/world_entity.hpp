#pragma once

#include "entity_map.hpp"
#include "world_system.hpp"

#include <functional>
#include <vector>

/// @brief The one entity that represents the world. Holds entities and world systems.
class WorldEntity
{
  private:
    EntityMap m_entityMap;
    std::vector<IWorldSystem*> m_systems;

  public:
    /// @brief 2 phases: loading and updating.
    /// @todo: better explanations (when it's donezo)
    void Update(ObjectModel::UpdateContext const& context)
    {
        // --------------
        // Loading phase
        // --------------

        // Register callbacks to propagate component registrations to world systems
        ObjectModel::LoadingContext loadingContext;
        loadingContext.m_registerWithWorldSystems = std::bind(&WorldEntity::RegisterComponent, *this, std::placeholders::_1, std::placeholders::_2);
        loadingContext.m_unregisterWithWorldSystems = std::bind(&WorldEntity::UnregisterComponent, *this, std::placeholders::_1, std::placeholders::_2);
        loadingContext.m_registerEntityUpdate = std::bind(&WorldEntity::RegisterEntity, *this, std::placeholders::_1);
        loadingContext.m_unregisterEntityUpdate = std::bind(&WorldEntity::UnregisterEntity, *this, std::placeholders::_1);
        // TODO: In order to paralellize, the entity registering functions could be bound from transient instances of the entity map.

        if (!m_entityMap.Load(loadingContext))
        {
            return; // Not all entities are loaded yet, return.
        }

        assert(m_entityMap.m_status == EntityMap::Status::Loaded);

        if (!m_entityMap.IsActivated())
        {
            m_entityMap.Activate(loadingContext);
        }

        // --------------
        // Updating phase
        // --------------

        // TODO: Update all systems for each entity
        // i.e. call entity.Update(context) for everyone.
        // Maybe like this ?
        // EntityCollection::Update(context);

        // TODO: Refine. For now a world update simply means updating all systems
        for (auto& system : m_systems)
        {
            system->Update(context);
        }
    }

    void RegisterComponent(Entity* pEntity, IComponent* pComponent)
    {
        std::cout << "Register component for entity: " << pEntity->GetName() << std::endl;
        for (auto& system : m_systems)
        {
            system->RegisterComponent(pEntity, pComponent);
        }
    }

    void UnregisterComponent(Entity* pEntity, IComponent* pComponent)
    {
        std::cout << "Unregister component for entity: " << pEntity->GetName() << std::endl;
        for (auto& system : m_systems)
        {
            system->UnregisterComponent(pEntity, pComponent);
        }
    }

    void RegisterEntity(Entity* pEntity)
    {
        std::cout << "Entity registered w/ world systems: " << pEntity->GetName() << std::endl;
        // TODO
    }

    void UnregisterEntity(Entity* pEntity)
    {
        std::cout << "Entity unregistered w/ world systems: " << pEntity->GetName() << std::endl;
        // TODO
    }

    template <typename T, class... Args>
    void CreateSystem(Args... args)
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");
        T* system = new T(args...);
        m_systems.push_back(system);
        // TODO: !! Handle deletion on destruction.
    }
};