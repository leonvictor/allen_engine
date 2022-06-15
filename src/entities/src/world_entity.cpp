#include "world_entity.hpp"
#include "component.hpp"
#include "entity.hpp"

#include <Tracy.hpp>
#include <assert.h>
#include <execution>
#include <functional>

namespace aln::entities
{
LoadingContext WorldEntity::GetLoadingContext()
{
    // Register callbacks to propagate component registrations to world systems
    LoadingContext loadingContext;
    loadingContext.m_registerWithWorldSystems = std::bind(&WorldEntity::RegisterComponent, this, std::placeholders::_1, std::placeholders::_2);
    loadingContext.m_unregisterWithWorldSystems = std::bind(&WorldEntity::UnregisterComponent, this, std::placeholders::_1, std::placeholders::_2);
    loadingContext.m_registerEntityUpdate = std::bind(&WorldEntity::RegisterEntityUpdate, this, std::placeholders::_1);
    loadingContext.m_unregisterEntityUpdate = std::bind(&WorldEntity::UnregisterEntityUpdate, this, std::placeholders::_1);
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
    struct UpdateTask
    {

        typedef std::list<Entity>::iterator iter;

        iter m_begin;
        iter m_end;
        UpdateContext m_updateContext;
        int m_threadNumber;

        UpdateTask(iter begin, iter end, UpdateContext updateContext, int thread_number)
            : m_begin(begin), m_end(end), m_updateContext(updateContext), m_threadNumber(thread_number) {}

        void operator()()
        {
            // tracy::SetThreadName(fmt::format("System updates ({})", m_threadNumber).c_str());
            for (auto it = m_begin; it != m_end; it++)
            {
                // TODO: Customize the context to allow further steps to populate a thread-specific map
                // TODO: When we join, we need to populate all of the maps
                it->UpdateSystems(m_updateContext);
            }
        }
    };

    ZoneScoped;
    // --------------
    // Loading phase
    // --------------

    auto loadingContext = GetLoadingContext();

    if (!m_entityMap.Load(loadingContext))
    {
        return; // Not all entities are loaded yet, return.
    }

    // assert(m_entityMap.m_status == EntityMap::Status::Loaded);

    if (!m_entityMap.IsActivated())
    {
        m_entityMap.Activate(loadingContext);
    }

    // --------------
    // Updating phase
    // --------------

    // Update all systems for each entity
    // TODO: Refine/parallelize

    const int num_threads = std::thread::hardware_concurrency();

    int grainsize = m_entityMap.m_entities.size() / num_threads;
    if (grainsize < 1)
        grainsize = 1;

    auto work_iter = std::begin(m_entityMap.m_entities);
    std::vector<UpdateTask> tasks;
    tasks.reserve(num_threads);

    for (uint8_t i = 0; i != num_threads - 1 && work_iter != m_entityMap.m_entities.end(); i++)
    {
        UpdateContext threadContext = context;

        auto end = std::next(work_iter, grainsize);
        tasks.push_back(UpdateTask(work_iter, end, threadContext, i));
        work_iter = end;
    }

    // The remaining systems could update in the main thread maybe ?
    tasks.push_back(UpdateTask(work_iter, m_entityMap.m_entities.end(), context, tasks.size()));

    std::for_each(std::execution::par, tasks.begin(), tasks.end(), [](auto& task)
        { task(); });

    tasks.clear();

    // TODO: Refine. For now a world update simply means updating all systems
    for (auto& [id, system] : m_systems)
    {
        system->Update(context);
    }
}

void WorldEntity::RegisterComponent(Entity* pEntity, IComponent* pComponent)
{
    // TODO: Create a task for each global system and feed them the entity/components pairs
    // TODO: Also delay registration till components are ready.
    // In the rare case multiple systems are interdependant, use the same thread for all of them.
    assert(!pComponent->IsUnloaded());
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

// std::map<aln::utils::UUID, Entity>& WorldEntity::GetEntitiesCollection()
// {
//     return m_entityMap.Collection();
// }

} // namespace aln::entities