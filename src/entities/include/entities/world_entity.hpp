#pragma once

#include "entity_map.hpp"
#include "world_system.hpp"

#include <functional>
#include <map>
#include <typeindex>
#include <typeinfo>

#include <utils/uuid.hpp>

namespace aln::entities
{
/// @brief The one entity that represents the world. Holds entities and world systems.
class WorldEntity
{
  private:
    EntityMap m_entityMap;
    std::map<std::type_index, IWorldSystem*> m_systems;

    /// @brief Build the loading context by registering callbacks to this world entity.
    LoadingContext GetLoadingContext();

    /// @brief Remove all entities and system from this world.
    void Cleanup();

  public:
    ~WorldEntity();

    /// @brief 2 phases: loading and updating.
    /// @todo: better explanations (when it's donezo)
    void Update(UpdateContext const& context);

    void RegisterComponent(Entity* pEntity, IComponent* pComponent);

    void UnregisterComponent(Entity* pEntity, IComponent* pComponent);

    void RegisterEntity(Entity* pEntity);

    void UnregisterEntity(Entity* pEntity);

    template <typename T, class... Args>
    void CreateSystem(Args... args)
    {
        static_assert(std::is_base_of_v<IWorldSystem, T>, "Invalid system type");
        T* system = new T(args...);
        system->InitializeSystem();
        m_systems.emplace(std::make_pair(std::type_index(typeid(T)), system));
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

    std::map<aln::utils::UUID, Entity>& GetEntitiesCollection();
    std::vector<Entity*>& GetEntityTree() { return m_entityMap.m_entitiesTree; }
};
} // namespace aln::entities