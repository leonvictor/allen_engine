#include "entity.hpp"

#include "entity_system.hpp"

#include "component.hpp"
#include "spatial_component.hpp"

#include <future>
#include <reflection/services/type_registry_service.hpp>

namespace aln
{

Event<Entity*> Entity::EntityStateUpdatedEvent;

Entity::~Entity()
{
    assert(IsUnloaded());

    m_deferredActions.clear();

    for (auto& pSystem : m_systems)
    {
        aln::Delete(pSystem);
    }
    m_systems.clear();

    for (auto& pComponent : m_components)
    {
        aln::Delete(pComponent);
    }
    m_components.clear();
}

// -------------------------------------------------
// State management
// -------------------------------------------------

void Entity::HandleDeferredActions(const LoadingContext& loadingContext)
{
    // TODO: Maybe early out if the entity is in the "remove" list ?
    for (auto& action : m_deferredActions)
    {
        switch (action.m_type)
        {
        case EntityInternalStateAction::Type::AddComponent:
        {
            auto pParentComponent = GetSpatialComponent(action.m_ID);
            AddComponentDeferred(loadingContext, (IComponent*) action.m_ptr, pParentComponent);
            break;
        }

        case EntityInternalStateAction::Type::DestroyComponent:
        {
            DestroyComponentDeferred(loadingContext, (IComponent*) action.m_ptr);
            break;
        }

        case EntityInternalStateAction::Type::CreateSystem:
        {
            CreateSystemDeferred(loadingContext, (aln::reflect::TypeInfo*) action.m_ptr);
            break;
        }

        case EntityInternalStateAction::Type::DestroySystem:
        {
            DestroySystemDeferred(loadingContext, (aln::reflect::TypeInfo*) action.m_ptr);
            break;
        }

        default:
            assert(false);
        }
    }

    m_deferredActions.clear();
}

bool Entity::UpdateLoadingAndEntityState(const LoadingContext& loadingContext)
{
    HandleDeferredActions(loadingContext);
    // TODO: Maybe fire entity state updated event here rather than in each separate action

    for (auto pComponent : m_components)
    {
        if (pComponent->IsUnloaded())
        {
            pComponent->LoadComponent(loadingContext);
        }

        if (pComponent->IsLoading())
        {
            if (!pComponent->UpdateLoadingStatus())
            {
                return false;
            }
        }

        if (pComponent->IsLoaded())
        {
            pComponent->InitializeComponent();
            if (IsActivated())
            {
                RegisterComponentWithLocalSystems(pComponent);
                loadingContext.m_registerWithWorldSystems(this, pComponent);
            }
        }
    }

    return true;
}

void Entity::LoadComponents(const LoadingContext& loadingContext)
{
    assert(m_status == Status::Unloaded);

    for (auto pComponent : m_components)
    {
        if (pComponent->IsUnloaded())
        {
            pComponent->LoadComponent(loadingContext);
        }
    }

    m_status = Status::Loaded;
}

void Entity::UnloadComponents(const LoadingContext& loadingContext)
{
    assert(m_status == Status::Loaded);

    for (auto pComponent : m_components)
    {
        if (pComponent->IsInitialized())
        {
            pComponent->ShutdownComponent();
        }
        // TODO: Component *should* be loaded in all cases
        if (pComponent->IsLoaded() || pComponent->IsLoading())
        {
            // TODO: What do we do with loadingContext ?
            pComponent->UnloadComponent(loadingContext);
        }
    }

    m_status = Status::Unloaded;
}

void Entity::Activate(const LoadingContext& loadingContext)
{
    ZoneScoped;

    assert(IsLoaded());
    m_status = Status::Activated;

    // Initialize spatial hierachy
    // TODO: Transforms are set at serial time so we have all the info available to update
    if (IsSpatialEntity())
    {
        // Calculate the initial world transform but do not trigger the callback to the component
        m_pRootSpatialComponent->CalculateWorldTransform(false);
    }

    for (auto pComponent : m_components)
    {
        if (pComponent->IsInitialized())
        {
            RegisterComponentWithLocalSystems(pComponent);
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

    loadingContext.m_registerEntityUpdate(this);
}

void Entity::Deactivate(const LoadingContext& loadingContext)
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
        if (pComponent->IsRegisteredWithEntitySystems())
        {
            UnregisterComponentWithLocalSystems(pComponent);
        }
        if (pComponent->IsRegisteredWithWorldSystems())
        {
            loadingContext.m_unregisterWithWorldSystems(this, pComponent);
        }
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
        auto comparator = [i](const IEntitySystem* pSystemA, const IEntitySystem* pSystemB)
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
void Entity::CreateSystem(const aln::reflect::TypeInfo* pTypeInfo)
{
    if (IsUnloaded())
    {
        CreateSystemImmediate(pTypeInfo);
    }
    else
    {
        // Delegate the action to whoever is in charge
        auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
        action.m_type = EntityInternalStateAction::Type::CreateSystem;
        action.m_ptr = pTypeInfo;

        EntityStateUpdatedEvent.Fire(this);
    }
}

void Entity::CreateSystemImmediate(const aln::reflect::TypeInfo* pSystemTypeInfo)
{
    assert(pSystemTypeInfo != nullptr);
    // TODO: assert that pSystemTypeInfo describes a type that is derived from IEntitySystem
    // TODO: DEBUG: Disable this loop in release builds

    // Make sure we only have one system of this type
    for (auto pExistingSystem : m_systems)
    {
        auto const pExistingSystemTypeInfo = pExistingSystem->GetTypeInfo();
        // TODO: Add inheritance info to the reflection system and put back the test.
        // if (pSystemTypeInfo->IsDerivedFrom(pExistingSystemTypeInfo->m_ID) || pSystemTypeInfo == pExistingSystemTypeInfo)
        if (pSystemTypeInfo == pExistingSystemTypeInfo)
        {
            throw std::runtime_error("Tried to add a second system of the same type to an entity.");
        }
    }

    auto pSystem = pSystemTypeInfo->CreateTypeInstance<IEntitySystem>();

    if (IsActivated())
    {
        for (auto pComponent : m_components)
        {
            pSystem->RegisterComponent(pComponent);
        }
    }

    m_systems.push_back(std::move(pSystem));
}

void Entity::CreateSystemDeferred(const LoadingContext& loadingContext, const aln::reflect::TypeInfo* pSystemTypeInfo)
{
    CreateSystemImmediate(pSystemTypeInfo);
    GenerateSystemUpdateList();

    // If already activated, notify the world systems that this entity update requirements changed
    if (IsActivated())
    {
        loadingContext.m_unregisterEntityUpdate(this);
        loadingContext.m_registerEntityUpdate(this);
    }
}

void Entity::DestroySystem(const aln::reflect::TypeInfo* pTypeInfo)
{
    assert(std::find_if(m_systems.begin(), m_systems.end(), [&](auto& pSystem)
               { return pSystem->GetTypeInfo() == pTypeInfo; }) != m_systems.end());

    if (IsUnloaded())
    {
        DestroySystemImmediate(pTypeInfo);
    }
    else
    {
        auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
        action.m_type = EntityInternalStateAction::Type::DestroySystem;
        action.m_ptr = pTypeInfo;

        EntityStateUpdatedEvent.Fire(this);
    }
}

void Entity::DestroySystemImmediate(const aln::reflect::TypeInfo* pSystemTypeInfo)
{
    auto it = std::find_if(m_systems.begin(), m_systems.end(), [&](auto& pSystem)
        { return pSystem->GetTypeInfo() == pSystemTypeInfo; });

    assert(it != m_systems.end());
    aln::Delete(*it);
    m_systems.erase(it);
}

void Entity::DestroySystemDeferred(const LoadingContext& loadingContext, const aln::reflect::TypeInfo* pSystemTypeInfo)
{
    DestroySystemImmediate(pSystemTypeInfo);
    GenerateSystemUpdateList();

    // If already activated, notify the world systems that this entity update requirements changed.
    if (IsActivated())
    {
        loadingContext.m_unregisterEntityUpdate(this);
        loadingContext.m_registerEntityUpdate(this);
    }
}

void Entity::UpdateSystems(UpdateContext const& context) const
{
    ZoneScoped;
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
void Entity::DestroyComponent(const UUID& componentID)
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
        EntityStateUpdatedEvent.Fire(this);
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

    aln::Delete(pComponent);
}

void Entity::DestroyComponentDeferred(const LoadingContext& loadingContext, IComponent* pComponent)
{
    if (IsActivated())
    {
        loadingContext.m_unregisterWithWorldSystems(this, pComponent);
        UnregisterComponentWithLocalSystems(pComponent);
    }

    if (pComponent->IsInitialized())
    {
        pComponent->ShutdownComponent();
    }

    if (pComponent->IsLoaded())
    {
        pComponent->UnloadComponent(loadingContext);
    }

    DestroyComponentImmediate(pComponent);
}

void Entity::AddComponent(IComponent* pComponent, const UUID& parentSpatialComponentID)
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
        EntityStateUpdatedEvent.Fire(this);
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
    m_components.push_back(pComponent);
}

void Entity::AddComponentDeferred(const LoadingContext& loadingContext, IComponent* pComponent, SpatialComponent* pParentComponent)
{
    AddComponentImmediate(pComponent, pParentComponent);
    pComponent->LoadComponent(loadingContext);
}

void Entity::RegisterComponentWithLocalSystems(IComponent* pComponent)
{
    assert(!pComponent->m_registeredWithEntitySystems);
    for (auto pSystem : m_systems)
    {
        pSystem->RegisterComponent(pComponent);
    }
    pComponent->m_registeredWithEntitySystems = true;
}

void Entity::UnregisterComponentWithLocalSystems(IComponent* pComponent)
{
    assert(pComponent->m_registeredWithEntitySystems);
    for (auto pSystem : m_systems)
    {
        pSystem->UnregisterComponent(pComponent);
    }
    pComponent->m_registeredWithEntitySystems = false;
}

// -------------------------------------------------
// Spatial stuff
// -------------------------------------------------

SpatialComponent* Entity::FindSocketAttachmentComponent(SpatialComponent* pComponentToSearch, const UUID& socketID) const
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

SpatialComponent* Entity::GetSpatialComponent(const UUID& spatialComponentID)
{
    if (!spatialComponentID.IsValid())
    {
        return nullptr;
    }

    auto componentIt = std::find_if(m_components.begin(), m_components.end(), [spatialComponentID](IComponent* comp)
        { return comp->GetID() == spatialComponentID; });
    assert(componentIt != m_components.end());

    auto pSpatialComponent = dynamic_cast<SpatialComponent*>(m_components[componentIt - m_components.begin()]);
    assert(pSpatialComponent != nullptr);
    return pSpatialComponent;
}

void Entity::RemoveChild(Entity* pEntity)
{
    auto it = std::find(m_attachedEntities.begin(), m_attachedEntities.end(), pEntity);
    assert(it != m_attachedEntities.end());
    m_attachedEntities.erase(it);
}

void Entity::AddChild(Entity* pEntity)
{
    m_attachedEntities.push_back(pEntity);
}

void Entity::SetParentEntity(Entity* pEntity)
{
    if (m_pParentSpatialEntity == pEntity)
        return;

    assert(IsSpatialEntity());

    // If the entity already had a parent, remove it
    bool hadParent = m_pParentSpatialEntity != nullptr;
    if (hadParent)
    {
        if (IsActivated())
            DetachFromParent();

        m_pParentSpatialEntity->RemoveChild(this);
    }

    // Set the new parent
    m_pParentSpatialEntity = pEntity;

    if (pEntity != nullptr)
    {
        assert(pEntity->IsSpatialEntity());
        pEntity->AddChild(this);
    }

    if (IsActivated())
    {

        if (pEntity != nullptr)
        {
            AttachToParent();
        }
    }
}

void Entity::AttachToParent()
{
    assert(IsSpatialEntity());
    assert(m_pParentSpatialEntity != nullptr && !m_isAttachedToParent);

    // Find component to attach to
    SpatialComponent* pParentRootComponent = m_pParentSpatialEntity->m_pRootSpatialComponent;

    // TODO: Set parentAttachmentSocketID when we set the parent entity
    if (m_parentAttachmentSocketID.IsValid())
    {
        if (auto pFoundComponent = m_pParentSpatialEntity->FindSocketAttachmentComponent(m_parentAttachmentSocketID))
        {
            pParentRootComponent = pFoundComponent;
        }
        else
        {
            // TODO: warning: no attachment socked id .. on entity ...
        }
    }

    // Perform attachment
    assert(pParentRootComponent != nullptr);
    m_pRootSpatialComponent->AttachTo(pParentRootComponent, m_parentAttachmentSocketID);

    m_isAttachedToParent = true;
}

void Entity::DetachFromParent()
{
    assert(IsSpatialEntity());
    assert(m_pParentSpatialEntity != nullptr && m_isAttachedToParent);

    m_pRootSpatialComponent->Detach();
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

// ---------- Live Editing. TODO: Disable in prod
void Entity::StartComponentEditing(const LoadingContext& loadingContext, IComponent* pComponent)
{
    assert(pComponent != nullptr);

    // Deactivate
    if (pComponent->IsRegisteredWithEntitySystems())
    {
        UnregisterComponentWithLocalSystems(pComponent);
    }
    if (pComponent->IsRegisteredWithWorldSystems())
    {
        loadingContext.m_unregisterWithWorldSystems(this, pComponent);
    }

    // Unload
    if (!pComponent->IsUnloaded())
    {
        if (pComponent->IsInitialized())
        {
            pComponent->ShutdownComponent();
        }
        pComponent->UnloadComponent(loadingContext);
    }
}

void Entity::EndComponentEditing(const LoadingContext& loadingContext)
{
    for (auto pComponent : m_components)
    {
        if (pComponent->IsUnloaded())
        {
            pComponent->Load(loadingContext);
        }
    }
}
} // namespace aln