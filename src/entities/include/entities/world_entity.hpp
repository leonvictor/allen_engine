#pragma once

#include "entity_map.hpp"
#include "world_system.hpp"

#include <functional>
#include <map>
#include <typeindex>
#include <typeinfo>

#include <common/services/service_provider.hpp>
#include <utils/uuid.hpp>

namespace aln
{
class Engine;
namespace editor
{
class Editor;
}

/// @brief The one entity that represents the world. Holds entities and world systems.
class WorldEntity
{
    friend class aln::Engine;
    friend class aln::editor::Editor;

  private:
    EntityMap m_entityMap;
    std::map<std::type_index, std::unique_ptr<IWorldSystem>> m_systems;

    TaskService* m_pTaskService = nullptr;

    LoadingContext m_loadingContext;

    /// @brief Remove all entities and system from this world.
    void Cleanup();

    /// @brief Register a component with all the world systems. Called when an entity is activated.
    void RegisterComponent(Entity* pEntity, IComponent* pComponent);

    /// @brief Unregister a component from all the world systems. Called when an entity is deactivated.
    void UnregisterComponent(Entity* pEntity, IComponent* pComponent);

    /// @brief Register an entity's update priorities list to the world. Called when an entity is activated or modified.
    void RegisterEntityUpdate(Entity* pEntity);

    /// @brief Unegister an entity's update priorities list from the world. Called when an entity is activated or modified.
    void UnregisterEntityUpdate(Entity* pEntity);

  public:
    ~WorldEntity();

    void Initialize(ServiceProvider& serviceProvider);

    /// @brief Update all entities' systems, then all world systems
    void Update(const UpdateContext& context);

    /// @brief Run the world's loading step, handling entities that were modified during the last frame
    void UpdateLoading();

    /// @brief Turn on an entity in the world.
    void ActivateEntity(Entity* pEntity);

    /// @brief Turn off an entity in the world.
    void DeactivateEntity(Entity* pEntity);

    template <typename T, class... Args>
    void CreateSystem(Args... args)
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");
        std::unique_ptr<T> system = std::make_unique<T>(args...);
        system->InitializeSystem();
        m_systems.emplace(std::make_pair(std::type_index(typeid(T)), std::move(system)));
    }

    template <typename T>
    void RemoveSystem()
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");

        auto& iter = m_systems.find(std::type_index(typeid(T)));
        if (iter != m_systems.end())
        {
            iter->second->ShutdownSystem();
            m_systems.erase(iter->first);
        }
    }

    const std::vector<Entity*>& GetEntities() const { return m_entityMap.m_entities; }
};
} // namespace aln