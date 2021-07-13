#pragma once

#include "entity_map.hpp"
#include "world_system.hpp"

#include <functional>
#include <map>
#include <typeinfo>

/// @brief The one entity that represents the world. Holds entities and world systems.
class WorldEntity
{
  private:
    EntityMap m_entityMap;
    std::map<std::type_index, IWorldSystem*> m_systems;

    /// @brief Build the loading context by registering callbacks to this world entity.
    ObjectModel::LoadingContext GetLoadingContext()
    {
        // Register callbacks to propagate component registrations to world systems
        ObjectModel::LoadingContext loadingContext;
        loadingContext.m_registerWithWorldSystems = std::bind(&WorldEntity::RegisterComponent, this, std::placeholders::_1, std::placeholders::_2);
        loadingContext.m_unregisterWithWorldSystems = std::bind(&WorldEntity::UnregisterComponent, this, std::placeholders::_1, std::placeholders::_2);
        loadingContext.m_registerEntityUpdate = std::bind(&WorldEntity::RegisterEntity, this, std::placeholders::_1);
        loadingContext.m_unregisterEntityUpdate = std::bind(&WorldEntity::UnregisterEntity, this, std::placeholders::_1);
        // TODO: In order to paralellize, the entity registering functions could be bound from transient instances of the entity map.
        return loadingContext;
    }

  public:
    ~WorldEntity()
    {
        Cleanup();
    }

    /// @brief Remove all entities and system from this world.
    void Cleanup()
    {
        auto loadingContext = GetLoadingContext();
        m_entityMap.Clear(loadingContext);

        for (auto& [id, system] : m_systems)
        {
            system->Shutdown();
        }
        m_systems.clear();
    }

    /// @brief 2 phases: loading and updating.
    /// @todo: better explanations (when it's donezo)
    void Update(ObjectModel::UpdateContext const& context)
    {
        // --------------
        // Loading phase
        // --------------

        auto loadingContext = GetLoadingContext();

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
        for (auto& [id, system] : m_systems)
        {
            system->Update(context);
        }
    }

    void RegisterComponent(Entity* pEntity, IComponent* pComponent)
    {
        std::cout << "Register component for entity: " << pEntity->GetName() << std::endl;
        for (auto& [id, system] : m_systems)
        {
            system->RegisterComponent(pEntity, pComponent);
        }
    }

    void UnregisterComponent(Entity* pEntity, IComponent* pComponent)
    {
        std::cout << "Unregister component for entity: " << pEntity->GetName() << std::endl;
        for (auto& [id, system] : m_systems)
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
        system->InitializeSystem();
        m_systems.emplace(std::make_pair(std::type_index(typeid(T)), system));
        // TODO: !! Handle deletion on destruction.
    }

    template <typename T>
    void RemoveSystem()
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");

        auto& iter = m_systems.find(std::type_index(typeid(T)));
        if (iter != m_systems.end())
        {
            iter->second->ShutdownSystem();
            delete iter->second;
            m_systems.erase(iter->first);
        }
    }
};