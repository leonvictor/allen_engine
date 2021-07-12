#include "entity.hpp"

#include "entity_collection.hpp"
#include "entity_system.hpp"

#include "component.hpp"
#include "spatial_component.hpp"

#include <future>

Command Entity::EntityStateUpdatedEvent;

// -------------------------------------------------
// State management
// -------------------------------------------------

/// @brief Check the loading status of all components and updates the entity status if necessary.
/// @return Whether the loading is finished
/// @todo This is synchrone for now
bool Entity::UpdateLoadingAndEntityState(const ObjectModel::LoadingContext& loadingContext)
{
    assert(m_status == Status::Unloaded);
    bool allLoaded = true;

    for (auto pComponent : m_components)
    {
        if (pComponent->IsUnloaded() || pComponent->IsLoading())
        {
            if (pComponent->LoadComponentAsync())
            {
                pComponent->InitializeComponent();
                assert(pComponent->IsInitialized());
            }
            else
            {
                allLoaded = false;
            }
        }
    }

    if (allLoaded)
    {
        m_status = Status::Loaded;
    }

    return allLoaded;
}

void Entity::LoadComponents(const ObjectModel::LoadingContext& loadingContext)
{
    assert(m_status == Status::Unloaded);

    for (auto pComponent : m_components)
    {
        if (pComponent->IsUnloaded())
        {
            // TODO: What do we do with loadingContext ?
            pComponent->Load();
        }
    }

    m_status = Status::Loaded;
}

void Entity::UnloadComponents(const ObjectModel::LoadingContext& loadingContext)
{
    assert(m_status == Status::Loaded);

    for (auto pComponent : m_components)
    {
        // TODO: Component *should* be loaded in all cases
        if (pComponent->IsLoaded())
        {
            // TODO: What do we do with loadingContext ?
            pComponent->Unload();
        }
    }

    m_status = Status::Loaded;
}

void Entity::Activate(const ObjectModel::LoadingContext& loadingContext)
{
    assert(IsLoaded());

    // Initialize spatial hierachy
    // TODO: Transforms are set at serial time so we have all the info available to update
    if (IsSpatialEntity())
    {
        // Calculate the initial world transform but do not trigger the callback to the component
        // TODO: ?
        m_pRootSpatialComponent->CalculateWorldTransform(false);
    }

    for (auto pComponent : m_components)
    {
        if (pComponent->IsInitialized())
        {
            // Register each component with all local systems...
            for (auto pSystem : m_systems)
            {
                pSystem->RegisterComponent(pComponent);
            }

            // ... and all global systems
            loadingContext.m_registerWithWorldSystems(this, pComponent);
        }
    }

    // Create per-status local system update lists
    GenerateSystemUpdateList();

    // Creates (spatial) entity attachments if required
    // TODO: also create the scheduling info (which entity update before the other)
    if (IsSpatialEntity())
    {
        if (m_pParentSpatialEntity != nullptr)
        {
            AttachToParent();
        }

        RefreshEntityAttachments();
    }

    m_status = Status::Activated;
    loadingContext.m_registerEntityUpdate(this);
}

void Entity::Deactivate(const ObjectModel::LoadingContext& loadingContext)
{
    // Exact opposite of Activate
    assert(m_status == Status::Activated);

    loadingContext.m_unregisterEntityUpdate(this);

    // Spatial attachments
    if (m_isAttachedToParent)
    {
        DetachFromParent();
    }

    // Clear system update lists
    for (size_t i = 0; i < (size_t) UpdateStage::NumStages; i++)
    {
        m_systemUpdateLists[i].clear();
    }

    // Unregister components from systems
    for (auto pComponent : m_components)
    {
        for (auto pSystem : m_systems)
        {
            pSystem->UnregisterComponent(pComponent);
        }

        loadingContext.m_unregisterWithWorldSystems(this, pComponent);
    }

    m_status = Status::Loaded;
}

void Entity::GenerateSystemUpdateList()
{
    for (size_t i = 0; i < (size_t) UpdateStage::NumStages; i++)
    {
        m_systemUpdateLists[i].clear();

        for (auto& pSystem : m_systems)
        {
            if (pSystem->GetRequiredUpdatePriorities().IsUpdateStageEnabled((UpdateStage) i))
            {
                m_systemUpdateLists[i].push_back(pSystem);
            }
        }

        // Sort update list
        // TODO: what does "T* const& var" mean ?
        auto comparator = [i](std::shared_ptr<IEntitySystem> const& pSystemA, std::shared_ptr<IEntitySystem> const& pSystemB)
        {
            uint16_t A = pSystemA->GetRequiredUpdatePriorities().GetPriorityForStage((UpdateStage) i);
            uint16_t B = pSystemB->GetRequiredUpdatePriorities().GetPriorityForStage((UpdateStage) i);
            return A > B;
        };

        std::sort(m_systemUpdateLists[i].begin(), m_systemUpdateLists[i].end(), comparator);
    }
}

// -------------------------------------------------
// Systems
// -------------------------------------------------

void Entity::CreateSystemImmediate(TypeInfo<IEntitySystem>* pSystemTypeInfo)
{
    assert(pSystemTypeInfo != nullptr);
    // TODO: assert that pSystemTypeInfo describes a type that is derived from IEntitySystem
    // TODO: DEBUG: Disable this loop in release builds

    // Make sure we only have one system of this type
    for (auto pExistingSystem : m_systems)
    {
        auto const pExistingSystemTypeInfo = pExistingSystem->GetTypeInfo();
        if (pSystemTypeInfo->IsDerivedFrom(pExistingSystemTypeInfo->m_ID) || pSystemTypeInfo == pExistingSystemTypeInfo)
        {
            throw std::runtime_error("Tried to add a second system of the same type to an entity.");
        }
    }

    auto pSystem = std::static_pointer_cast<IEntitySystem>(pSystemTypeInfo->m_pTypeHelper->CreateType());
    // TODO: Initialize the typeInfo member in the creation routine.
    // TODO: Put back the const modifier to the pSystemTypeInfo arg.
    pSystem->m_pTypeInfo = pSystemTypeInfo;
    m_systems.push_back(pSystem);
}

// TODO: Put back const modifier when we've fixed the pSystemTypeInfo setter
void Entity::CreateSystemDeferred(const ObjectModel::LoadingContext& loadingContext, TypeInfo<IEntitySystem>* pSystemTypeInfo)
{
    CreateSystemImmediate(pSystemTypeInfo);
    GenerateSystemUpdateList();

    // If already activated, notify the world systems that this entity requires a reload
    if (IsActivated())
    {
        loadingContext.m_unregisterEntityUpdate(this);
        loadingContext.m_registerEntityUpdate(this);
    }
}

void Entity::DestroySystemImmediate(const TypeInfo<IEntitySystem>* pSystemTypeInfo)
{
    // TODO: find the index of system in m_systems

    auto systemIt = std::find_if(m_systems.begin(), m_systems.end(), [&](std::shared_ptr<IEntitySystem> pSystem)
        { return pSystem->GetTypeInfo()->m_ID == pSystemTypeInfo->m_ID; });
    // int systemIdx = 0;

    // TODO: assert that systemIdx is valid
    m_systems.erase(systemIt);
}

void Entity::DestroySystemDeferred(const ObjectModel::LoadingContext& loadingContext, const TypeInfo<IEntitySystem>* pSystemTypeInfo)
{
    DestroySystemImmediate(pSystemTypeInfo);
    GenerateSystemUpdateList();

    if (IsActivated())
    {
        loadingContext.m_unregisterEntityUpdate(this);
        loadingContext.m_registerEntityUpdate(this);
    }
}

void Entity::UpdateSystems(ObjectModel::UpdateContext const& context)
{
    const UpdateStage updateStage = context.GetUpdateStage();
    // TODO: make sure stages convert nicely to uint8_t
    for (auto pSystem : m_systemUpdateLists[(uint8_t) updateStage])
    {
        // TODO: assert system has this stage enabled
        assert(pSystem->GetRequiredUpdatePriorities().IsUpdateStageEnabled(updateStage));
        pSystem->Update(context);
    }
}

// -------------------------------------------------
// Components
// -------------------------------------------------
void Entity::DestroyComponent(const core::UUID& componentID)
{
    assert(componentID.IsValid());
    auto componentIt = std::find_if(m_components.begin(), m_components.end(), [componentID](IComponent* comp)
        { return comp->GetID() == componentID; });
    assert(componentIt != m_components.end());

    auto pComponent = m_components[componentIt - m_components.begin()];

    if (IsUnloaded())
    {
        // Root removal validation
        if (pComponent == m_pRootSpatialComponent)
        {
            // TODO: Assert that the spatial component has 1 child or less. Otherwise we need to detach the other children first.
            return;
        }

        DestroyComponentImmediate(pComponent);
    }
    else // Defer to the next loading phase
    {
        auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
        action.m_type = EntityInternalStateAction::Type::DestroyComponent;
        action.m_ptr = pComponent;

        // Notify the world system
        EntityStateUpdatedEvent.Execute(this);
    }
}

void Entity::DestroyComponentImmediate(IComponent* pComponent)
{
    assert(pComponent->m_entityID == m_ID);

    auto componentIt = std::find(m_components.begin(), m_components.end(), pComponent);
    assert(componentIt != m_components.end());

    m_components.erase(componentIt);

    SpatialComponent* pSpatialComponent = dynamic_cast<SpatialComponent*>(pComponent);
    if (pSpatialComponent != nullptr)
    {
        if (m_pRootSpatialComponent == pSpatialComponent)
        {
            m_pRootSpatialComponent = nullptr;
        }
        else
        {
            pSpatialComponent->Detach();
        }
    }

    pComponent->m_entityID = core::UUID::InvalidID;
    // TODO: Shutdown / Unload here ?
    pComponent->Shutdown();
    pComponent->Unload();

    // TODO: Experiment with different component storage modes
    // Using a pool might be a good idea, to pack them in contiguous memory regions
    delete pComponent;
}

void Entity::DestroyComponentDeferred(const ObjectModel::LoadingContext& context, IComponent* pComponent)
{
    DestroyComponentImmediate(pComponent);
    if (IsLoaded())
    {
        context.m_unregisterEntityUpdate(this);
        context.m_registerEntityUpdate(this);
    }
}

void Entity::AddComponent(IComponent* pComponent, const core::UUID& parentSpatialComponentID)
{
    assert(pComponent != nullptr && pComponent->GetID().IsValid());
    assert(!pComponent->m_entityID.IsValid() && pComponent->IsUnloaded());
    // TODO: Assert that m_components does NOT contain a component of this type

    SpatialComponent* pSpatialComponent = dynamic_cast<SpatialComponent*>(pComponent);

    // Parent ID can only be set when adding a spatial component
    if (pSpatialComponent == nullptr)
    {
        assert(!parentSpatialComponentID.IsValid()); // , "Tried to set a parent to a non-spatial component."
    }

    if (IsUnloaded())
    {
        // TODO: Replaced by a method, but we lost an assertion
        // SpatialComponent* pParentComponent = nullptr;
        // if (parentSpatialComponentID.IsValid())
        // {
        //     assert(pSpatialComponent != nullptr);

        //     auto componentIt = std::find(m_components.begin(), m_components.end(), [parentSpatialComponentID](IComponent* comp) { comp->GetID() == parentSpatialComponentID; });
        //     assert(componentIt != m_components.end());

        //     pParentComponent = dynamic_cast<SpatialComponent*>(m_components[componentIt - m_components.begin()]);
        //     assert(pParentComponent != nullptr);
        // }
        auto pParentComponent = GetSpatialComponent(parentSpatialComponentID);
        AddComponentImmediate(pComponent, pParentComponent);
    }
    else // Defer to the next loading phase
    {
        auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
        action.m_type = EntityInternalStateAction::Type::AddComponent;
        action.m_ptr = pComponent;
        action.m_ID = parentSpatialComponentID;

        // Send notification that the internal state changed
        EntityStateUpdatedEvent.Execute(this);
    }
}

void Entity::AddComponentImmediate(IComponent* pComponent, SpatialComponent* pParentComponent)
{
    SpatialComponent* pSpatialComponent = dynamic_cast<SpatialComponent*>(pComponent);
    if (pSpatialComponent != nullptr)
    {
        if (pParentComponent == nullptr)
        {
            if (m_pRootSpatialComponent == nullptr)
            {
                // The component becomes the root
                m_pRootSpatialComponent = pSpatialComponent;
            }
            else
            {
                // The component is attached to the root component.
                // Side effect: the entity becomes a spatial one !

                // TODO: manage sockets
                // TODO: do not allow circular dependencies
                pSpatialComponent->AttachTo(m_pRootSpatialComponent);
                // m_pRootSpatialComponent->m_spatialChildren.push_back(pSpatialComponent);
                // pSpatialComponent->m_pSpatialParent = m_pRootSpatialComponent;
            }
        }
        else
        {
            // TODO: assert that pSpatialComponent belongs to the same entity
            // The component is attached to the specified parent
            pSpatialComponent->AttachTo(pParentComponent);
            // pParentComponent->m_spatialChildren.push_back(pSpatialComponent);
            // pSpatialComponent->m_pSpatialParent = pParentComponent;
        }
    }

    pComponent->m_entityID = m_ID;
    // TODO: make sure modification made to pSpatialComponent are taken into account here...
    m_components.emplace_back(pComponent);
}

void Entity::AddComponentDeferred(const ObjectModel::LoadingContext& context, IComponent* pComponent, SpatialComponent* pParentComponent)
{
    AddComponentImmediate(pComponent, pParentComponent);

    // If resources have already been loaded, notify the world system that this entity requires a reload
    if (IsLoaded())
    {
        // TODO: should i use the same requests as for system ?
        context.m_unregisterEntityUpdate(this);
        context.m_registerEntityUpdate(this);
    }
}

// -------------------------------------------------
// Spatial stuff
// -------------------------------------------------

SpatialComponent* Entity::FindSocketAttachmentComponent(SpatialComponent* pComponentToSearch, const core::UUID& socketID) const
{
    assert(pComponentToSearch != nullptr);
    if (pComponentToSearch->HasSocket(socketID))
    {
        return pComponentToSearch;
    }

    for (auto pChildComponent : pComponentToSearch->m_spatialChildren)
    {
        if (auto pFoundComponent = FindSocketAttachmentComponent(pChildComponent, socketID))
        {
            return pFoundComponent;
        }
    }

    return nullptr;
}

SpatialComponent* Entity::GetSpatialComponent(const core::UUID& spatialComponentID)
{
    if (!spatialComponentID.IsValid())
    {
        return nullptr;
    }

    // assert(pSpatialComponent != nullptr);

    auto componentIt = std::find_if(m_components.begin(), m_components.end(), [spatialComponentID](IComponent* comp)
        { return comp->GetID() == spatialComponentID; });
    assert(componentIt != m_components.end());

    auto pSpatialComponent = dynamic_cast<SpatialComponent*>(m_components[componentIt - m_components.begin()]);
    assert(pSpatialComponent != nullptr);
    return pSpatialComponent;
}

void Entity::AttachToParent()
{
    assert(IsSpatialEntity());
    assert(m_pParentSpatialEntity != nullptr && !m_isAttachedToParent);

    // Find component to attach to
    auto pParentEntity = m_pParentSpatialEntity;
    SpatialComponent* pParentRootComponent = pParentEntity->m_pRootSpatialComponent;

    // TODO: Set parentAttachmentSocketID when we set the parent entity
    if (m_parentAttachmentSocketID.IsValid())
    {
        if (auto pFoundComponent = pParentEntity->FindSocketAttachmentComponent(m_parentAttachmentSocketID))
        {
            pParentRootComponent = pFoundComponent;
        }
        else
        {
            // TODO: warning: no attachment socked id .. on entity ...
        }
    }

    // Perform attachment
    m_pRootSpatialComponent->AttachTo(pParentRootComponent, m_parentAttachmentSocketID);

    // assert(pParentRootComponent != nullptr);

    // Set component hierarchy values
    // m_pRootSpatialComponent->m_pSpatialParent = pParentRootComponent;
    // m_pRootSpatialComponent->m_parentAttachmentSocketID = m_parentAttachmentSocketID;

    // TODO: should we calculate world transform everytime a component is attached to another ?
    // Or only here ?
    // m_pRootSpatialComponent->CalculateWorldTransform();

    // Add to the list of child components on the component to attach to
    // pParentRootComponent->m_spatialChildren.emplace_back(m_pRootSpatialComponent);

    m_isAttachedToParent = true;
}

void Entity::DetachFromParent()
{
    assert(IsSpatialEntity());
    assert(m_pParentSpatialEntity != nullptr && m_isAttachedToParent);

    // TODO: Is there a reason *not* to use component.Detach() ?
    // Remove from parent component child list
    m_pRootSpatialComponent->Detach();
    // auto pParentComponent = m_pRootSpatialComponent->m_pSpatialParent;
    // auto foundIter = std::find(pParentComponent->m_spatialChildren.begin(), pParentComponent->m_spatialChildren.end(), m_pRootSpatialComponent);
    // assert(foundIter != pParentComponent->m_spatialChildren.end());
    // pParentComponent->m_spatialChildren.erase(foundIter);

    // Remove component hierarchy values
    // m_pRootSpatialComponent->m_pSpatialParent = nullptr;
    // m_pRootSpatialComponent->m_parentAttachmentSocketID = core::UUID::InvalidID;

    m_isAttachedToParent = false;
}

void Entity::RefreshEntityAttachments()
{
    assert(IsSpatialEntity());
    for (auto pAttachedEntity : m_attachedEntities)
    {
        // Only refresh active attachments
        if (pAttachedEntity->m_isAttachedToParent)
        {
            pAttachedEntity->DetachFromParent();
            pAttachedEntity->AttachToParent();
        }
    }
}

Entity* Entity::Create(std::string name)
{
    Entity* pEntity = EntityCollection::Create();
    pEntity->m_name = name;
    // TODO: Make sure Id is generated and valid
    return pEntity;
}