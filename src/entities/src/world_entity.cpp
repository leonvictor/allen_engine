#include "world_entity.hpp"
#include "entity.hpp"

#include <assert.h>

namespace aln::entities
{
LoadingContext WorldEntity::GetLoadingContext()
{
    // Register callbacks to propagate component registrations to world systems
    LoadingContext loadingContext;
    loadingContext.m_registerWithWorldSystems = std::bind(&WorldEntity::RegisterComponent, this, std::placeholders::_1, std::placeholders::_2);
    loadingContext.m_unregisterWithWorldSystems = std::bind(&WorldEntity::UnregisterComponent, this, std::placeholders::_1, std::placeholders::_2);
    loadingContext.m_registerEntityUpdate = std::bind(&WorldEntity::RegisterEntity, this, std::placeholders::_1);
    loadingContext.m_unregisterEntityUpdate = std::bind(&WorldEntity::UnregisterEntity, this, std::placeholders::_1);
    // TODO: In order to paralellize, the entity registering functions could be bound from transient instances of the entity map.
    return loadingContext;
}

WorldEntity::~WorldEntity()
{
    Cleanup();
}

void WorldEntity::Cleanup()
{
    auto loadingContext = GetLoadingContext();
    m_entityMap.Clear(loadingContext);

    for (auto& [id, system] : m_systems)
    {
        system->Shutdown();
    }
    m_systems.clear();
}

void WorldEntity::Update(const UpdateContext& context)
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

void WorldEntity::RegisterComponent(Entity* pEntity, IComponent* pComponent)
{
    std::cout << "Register component for entity: " << pEntity->GetName() << std::endl;
    for (auto& [id, system] : m_systems)
    {
        system->RegisterComponent(pEntity, pComponent);
    }
}

void WorldEntity::UnregisterComponent(Entity* pEntity, IComponent* pComponent)
{
    std::cout << "Unregister component for entity: " << pEntity->GetName() << std::endl;
    for (auto& [id, system] : m_systems)
    {
        system->UnregisterComponent(pEntity, pComponent);
    }
}

void WorldEntity::RegisterEntity(Entity* pEntity)
{
    std::cout << "Entity registered w/ world systems: " << pEntity->GetName() << std::endl;
    // TODO
}

void WorldEntity::UnregisterEntity(Entity* pEntity)
{
    std::cout << "Entity unregistered w/ world systems: " << pEntity->GetName() << std::endl;
    // TODO
}
} // namespace aln::entities