#include "entity_map.hpp"

#include "entity.hpp"
#include "loading_context.hpp"
#include "object_model.hpp"

#include <reflection/reflection.hpp>

#include <algorithm>
#include <array>
#include <execution>
#include <functional>
#include <mutex>
#include <semaphore>
#include <stdexcept>
#include <thread>

#include <Tracy.hpp>
#include <common/TracySystem.hpp>
#include <fmt/core.h>

namespace aln::entities
{
void EntityMap::Clear(const LoadingContext& loadingContext)
{
    m_entitiesToRemove.clear();
    m_loadingEntities.clear();
    m_entitiesToReload.clear();

    // If this is the main map, deactivate and unload all entities,
    // then clear the collection.
    if (!m_isTransientMap)
    {
        for (auto& pEntity : m_entities)
        {
            if (pEntity->IsActivated())
            {
                pEntity->Deactivate(loadingContext);
            }
            pEntity->UnloadComponents(loadingContext);
        }
        m_entities.clear();
    }
}

void EntityMap::RemoveEntity(Entity* pEntity)
{
    m_entitiesToRemove.push_back(pEntity);
}

void EntityMap::ActivateEntity(Entity* pEntity)
{
    m_entitiesToActivate.push_back(pEntity);
}

void EntityMap::DeactivateEntity(Entity* pEntity)
{
    m_entitiesToDeactivate.push_back(pEntity);
}

bool EntityMap::Load(const LoadingContext& loadingContext)
{
    // Manage the events registered by entities outside of the loading phase
    // TODO: Where should this logic be ?
    for (auto* pEntity : Entity::EntityStateUpdatedEvent.m_updatedEntities)
    {
        // TODO: Maybe early out if the entity is in the "remove" list ?
        for (auto action : pEntity->m_deferredActions)
        {
            switch (action.m_type)
            {
            case EntityInternalStateAction::Type::ParentChanged:
            {
                auto it = std::find(m_entitiesTree.begin(), m_entitiesTree.end(), pEntity);
                if (pEntity->HasParentEntity()) // It shouldn't be at the tree root
                {
                    if (it != m_entitiesTree.end())
                    {
                        m_entitiesTree.erase(it);
                    }
                }
                else
                {
                    if (it == m_entitiesTree.end()) // Add it to the tree root if it previously had a parent
                    {
                        m_entitiesTree.push_back(pEntity);
                    } // Otherwise it's already in the right place
                }
                break;
            }

            case EntityInternalStateAction::Type::AddComponent:
            {
                auto pParentComponent = pEntity->GetSpatialComponent(action.m_ID);
                pEntity->AddComponentDeferred(loadingContext, (IComponent*) action.m_ptr, pParentComponent);
                break;
            }

            case EntityInternalStateAction::Type::DestroyComponent:
            {
                pEntity->DestroyComponentDeferred(loadingContext, (IComponent*) action.m_ptr);
                break;
            }

            case EntityInternalStateAction::Type::CreateSystem:
            {
                pEntity->CreateSystemDeferred(loadingContext, (aln::reflect::TypeDescriptor*) action.m_ptr);
                break;
            }

            case EntityInternalStateAction::Type::DestroySystem:
            {
                pEntity->DestroySystemDeferred(loadingContext, (aln::reflect::TypeDescriptor*) action.m_ptr);
                break;
            }

            default:
                throw std::runtime_error("Unsupported operation");
            }
        }
        pEntity->m_deferredActions.clear();
    }
    Entity::EntityStateUpdatedEvent.m_updatedEntities.clear();

    // Deactivate, unload and remove entities from the collection
    for (auto pEntityToRemove : m_entitiesToRemove)
    {
        // Remove from the tree view
        if (!pEntityToRemove->HasParentEntity())
        {
            auto it = std::find_if(m_entitiesTree.begin(), m_entitiesTree.end(), [&](Entity* pEntity)
                { return pEntityToRemove->GetID() == pEntity->GetID(); });
            assert(it != m_entitiesTree.end());
            m_entitiesTree.erase(it);
        }

        // Deactivate if activated
        if (pEntityToRemove->IsActivated())
        {
            pEntityToRemove->Deactivate(loadingContext);
        }
        else // Remove from loading lists as we might still be loading this entity
        {
            // TODO: Removing from vectors is meh. Profile and check if std::list/std::multimap are more efficient for what we do
            auto itLoading = std::find(m_loadingEntities.begin(), m_loadingEntities.end(), pEntityToRemove);
            if (itLoading != m_loadingEntities.end())
            {
                m_loadingEntities.erase(itLoading);
            }

            auto itReload = std::find(m_entitiesToReload.begin(), m_entitiesToReload.end(), pEntityToRemove);
            if (itReload != m_entitiesToReload.end())
            {
                m_entitiesToReload.erase(itReload);
            }
        }

        // Unload components and remove from collection
        pEntityToRemove->UnloadComponents(loadingContext);
        std::remove_if(m_entities.begin(), m_entities.end(), [&](Entity* pEntity)
            { return pEntity->GetID() == pEntityToRemove->m_ID; });
    }

    m_entitiesToRemove.clear();

    // Entity loading
    if (m_loadingEntities.size() > 0 && m_status == Status::Deactivated)
    {
        m_status = Status::EntitiesLoading;
    }

    std::vector<Entity*> stillLoadingEntities;
    for (auto pEntity : m_loadingEntities)
    {
        if (pEntity->UpdateLoadingAndEntityState(loadingContext))
        {
            // If the map is activated, immediately activate any entities that finish loading
            if (IsActivated() && !pEntity->IsActivated())
            {
                pEntity->Activate(loadingContext);

                if (!pEntity->HasParentEntity())
                {
                    m_entitiesTree.push_back(pEntity);
                }
            }
        }
        else // Entity is still loading
        {
            stillLoadingEntities.push_back(pEntity);
            // return false; // TODO ?
        }
    }
    m_loadingEntities = stillLoadingEntities;

    // Ensure that we set the status to loaded, if we were in the entity loading stage and all entities were successfully loaded
    // TODO:
    if (m_status == Status::EntitiesLoading && m_loadingEntities.empty())
    {
        assert(!m_isTransientMap);
        m_status = Status::Loaded;
    }

    // TODO: All entities are activated in parallel.
    // As the map will be instanciated for multiple threads this is ok for now i think ?
    // We can call entity->Activate() for each of them because it manages the local parts of the activation,
    // the rest is deffered.
    for (auto pEntity : m_entitiesToActivate)
    {
        pEntity->Activate(loadingContext);
    }

    m_entitiesToActivate.clear();

    for (auto pEntity : m_entitiesToDeactivate)
    {
        pEntity->Deactivate(loadingContext);
    }
    m_entitiesToDeactivate.clear();

    return true;
}

void EntityMap::Activate(const LoadingContext& loadingContext)
{
    // TODO: Parallel
    struct ActivationTask
    {
        /// ...
    };

    for (auto pEntity : m_entities)
    {
        if (pEntity->IsLoaded())
        {
            pEntity->Activate(loadingContext);

            if (!pEntity->HasParentEntity())
            {
                m_entitiesTree.push_back(pEntity);
            }
        }
    }
    m_status = Status::Activated;
}

Entity* EntityMap::CreateEntity(std::string name)
{
    std::lock_guard lock(m_mutex);

    auto pEntity = aln::New<Entity>();
    pEntity->m_name = name;
    m_entities.push_back(pEntity);
    // TODO: What's the condition ?
    // if (IsActivated())
    m_loadingEntities.push_back(pEntity);

    return pEntity;
}
} // namespace aln::entities