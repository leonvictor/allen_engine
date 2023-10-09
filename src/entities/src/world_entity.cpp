#include "world_entity.hpp"
#include "component.hpp"
#include "entity.hpp"

#include <assets/asset_service.hpp>
#include <common/threading/task_service.hpp>

#include <tracy/Tracy.hpp>

#include <assert.h>
#include <execution>
#include <functional>

namespace aln
{

void WorldEntity::Initialize(ServiceProvider& serviceProvider)
{
    m_pTaskService = serviceProvider.GetService<TaskService>();
    assert(m_pTaskService != nullptr);

    auto pAssetService = serviceProvider.GetService<AssetService>();
    assert(pAssetService != nullptr);

    m_loadingContext = LoadingContext(m_pTaskService, pAssetService);

    // Register callbacks to propagate component registrations to world systems
    m_loadingContext.m_registerWithWorldSystems = std::bind(&WorldEntity::RegisterComponent, this, std::placeholders::_1, std::placeholders::_2);
    m_loadingContext.m_unregisterWithWorldSystems = std::bind(&WorldEntity::UnregisterComponent, this, std::placeholders::_1, std::placeholders::_2);
    m_loadingContext.m_registerEntityUpdate = std::bind(&WorldEntity::RegisterEntityUpdate, this, std::placeholders::_1);
    m_loadingContext.m_unregisterEntityUpdate = std::bind(&WorldEntity::UnregisterEntityUpdate, this, std::placeholders::_1);

    assert(m_loadingContext.IsInitialized());

    m_entityMap.Load(m_loadingContext);
    m_entityMap.Activate(m_loadingContext);
}

WorldEntity::~WorldEntity()
{
    Cleanup();
}

void WorldEntity::Cleanup()
{
    for (auto& [id, pSystem] : m_systems)
    {
        pSystem->Shutdown();
        aln::Delete(pSystem);
    }
    m_systems.clear();
    m_entityMap.Clear(m_loadingContext);
}

void WorldEntity::Update(const UpdateContext& context)
{
    struct UpdateTask : public ITaskSet
    {
        const Vector<Entity*>& m_entities;
        const UpdateContext& m_updateContext;

        UpdateTask(const Vector<Entity*>& entities, const UpdateContext& updateContext)
            : ITaskSet(entities.size()), m_entities(entities), m_updateContext(updateContext) {}

        virtual void ExecuteRange(TaskSetPartition range, uint32_t threadNum) override
        {
            for (auto i = range.start; i < range.end; ++i)
            {
                const auto pEntity = m_entities[i];
                pEntity->UpdateSystems(m_updateContext);
            }
        }
    };

    ZoneScoped;

    // --------------
    // Loading phase
    // --------------

    assert(m_loadingContext.IsInitialized());
    m_entityMap.UpdateEntitiesState(m_loadingContext);

    // --------------
    // Updating phase
    // --------------

    // Update all systems for each entity
    auto updateTask = UpdateTask(m_entityMap.m_entities, context);
    m_pTaskService->ExecuteTask(&updateTask);

    // TODO: Refine. For now a world update simply means updating all systems
    for (auto& [id, system] : m_systems)
    {
        system->Update(context);
    }
}

void WorldEntity::UpdateLoading()
{
    m_entityMap.UpdateEntitiesState(m_loadingContext);
}

void WorldEntity::RegisterComponent(Entity* pEntity, IComponent* pComponent)
{
    // TODO: Create a task for each global system and feed them the entity/components pairs
    // TODO: Also delay registration till components are ready.
    // In the rare case multiple systems are interdependant, use the same thread for all of them.
    assert(!pComponent->IsUnloaded());
    assert(!pComponent->m_registeredWithWorldSystems);
    for (auto& [id, system] : m_systems)
    {
        system->RegisterComponent(pEntity, pComponent);
    }
    pComponent->m_registeredWithWorldSystems = true;
}

void WorldEntity::UnregisterComponent(Entity* pEntity, IComponent* pComponent)
{
    assert(pComponent->m_registeredWithWorldSystems);
    for (auto& [id, system] : m_systems)
    {
        system->UnregisterComponent(pEntity, pComponent);
    }
    pComponent->m_registeredWithWorldSystems = false;
}

void WorldEntity::RegisterEntityUpdate(Entity* pEntity)
{
    // TODO
    std::cout << "Entity update list registered w/ world: " << pEntity->GetName() << std::endl;
}

void WorldEntity::UnregisterEntityUpdate(Entity* pEntity)
{
    // TODO
    std::cout << "Entity update list unregistered w/ world: " << pEntity->GetName() << std::endl;
}

void WorldEntity::ActivateEntity(Entity* pEntity)
{
    m_entityMap.ActivateEntity(pEntity);
}

void WorldEntity::DeactivateEntity(Entity* pEntity)
{
    m_entityMap.DeactivateEntity(pEntity);
}

} // namespace aln