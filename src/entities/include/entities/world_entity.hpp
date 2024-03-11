#pragma once

#include "entity_map.hpp"
#include "world_system.hpp"

#include <common/services/service_provider.hpp>
#include <common/uuid.hpp>
#include <graphics/viewport.hpp>

#include <typeindex>
#include <typeinfo>

namespace aln
{

class IComponent;

/// @brief Holds entities and world systems.
class WorldEntity
{
    friend class Engine;
    friend class Editor;
    friend class EntityInspector;

  private:
    EntityMap m_entityMap;
    HashMap<std::type_index, IWorldSystem*, std::hash<std::type_index>> m_systems;

    ServiceProvider* m_pServiceProvider = nullptr;
    TaskService* m_pTaskService = nullptr;
    Viewport m_viewport;

    LoadingContext m_loadingContext;

    /// @brief Register a component with all the world systems. Called when an entity is activated.
    void RegisterComponent(Entity* pEntity, IComponent* pComponent);

    /// @brief Unregister a component from all the world systems. Called when an entity is deactivated.
    void UnregisterComponent(Entity* pEntity, IComponent* pComponent);

    /// @brief Register an entity's update priorities list to the world. Called when an entity is activated or modified.
    void RegisterEntityUpdate(Entity* pEntity);

    /// @brief Unegister an entity's update priorities list from the world. Called when an entity is activated or modified.
    void UnregisterEntityUpdate(Entity* pEntity);

  public:
    void Initialize(ServiceProvider& serviceProvider);
    void Shutdown();

    /// @brief Update all entities' systems, then all world systems
    void Update(const UpdateContext& context);

    /// @brief Run the world's loading step, handling entities that were modified during the last frame
    void UpdateLoading();

    /// @brief Create a new entity and add it to the world
    /// @todo Replace the string name parameter
    Entity* CreateEntity(std::string entityName) { return m_entityMap.CreateEntity(entityName); }

    /// @brief Turn on an entity in the world.
    void ActivateEntity(Entity* pEntity);

    /// @brief Turn off an entity in the world.
    void DeactivateEntity(Entity* pEntity);

    template <typename T, class... Args>
    void CreateSystem(Args... args)
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");
        assert(m_pServiceProvider != nullptr);

        // TODO: Prevent creating multiple systems of the same type
        auto pSystem = aln::New<T>(args...);
        pSystem->InitializeSystem(*m_pServiceProvider);
        m_systems.emplace(std::type_index(typeid(T)), pSystem);
    }

    template <typename T>
    void RemoveSystem()
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");
        assert(m_pServiceProvider != nullptr);

        auto& iter = m_systems.find(std::type_index(typeid(T)));
        if (iter != m_systems.end())
        {
            auto pSystem = iter->second;
            pSystem->ShutdownSystem(*m_pServiceProvider);
            aln::Delete(pSystem);
            m_systems.erase(iter->first);
        }
    }

    template <typename T>
    T* GetSystem() 
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");

        auto iter = m_systems.find(std::type_index(typeid(T)));
        assert(iter != m_systems.end());
        return static_cast<T*>(iter->second);
    }

    template <typename T>
    const T* GetSystem() const
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");

        auto iter = m_systems.find(std::type_index(typeid(T)));
        assert(iter != m_systems.end());
        return static_cast<T*>(iter->second);
    }

    const Vector<Entity*>& GetEntities() const { return m_entityMap.m_entities; }

    void InitializeViewport(const Rectangle& size) { m_viewport.m_size = size; }
    const Viewport* GetViewport() const { return &m_viewport; }
    
    // -------- Editing
    // TODO: Disable in prod

    void StartComponentEditing(IComponent* pComponent)
    {
        assert(pComponent != nullptr);
        m_entityMap.StartComponentEditing(m_loadingContext, pComponent);
    }

    void EndComponentEditing(IComponent* pComponent) {}
};
} // namespace aln