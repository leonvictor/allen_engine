#include "entity_map.hpp"
#include "world_system.cpp"

#include <vector>

/// @brief The one entity that represents the world. Holds entities and world systems.
class WorldEntity
{
  private:
    EntityMap m_entityMap;
    std::vector<IWorldSystem> m_systems;

  public:
    void Update(ObjectModel::UpdateContext const& context)
    {
        ObjectModel::LoadingContext loadingContext; // TODO
        m_entityMap.Update(loadingContext);

        // TODO: Update all systems for each entity
        // i.e. call entity.Update(context) for everyone.

        // Maybe like this ?
        // EntityCollection::Update(context);

        // TODO: Refine. For now a world update simply means updating all systems
        for (auto& system : m_systems)
        {
            system.Update(context);
        }
    }
};