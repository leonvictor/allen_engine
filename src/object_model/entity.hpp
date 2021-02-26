#pragma once

#include "object_model.cpp"
#include "spatial_component.cpp"

#include <assert.h>
#include <stdexcept>
#include <vector>

#include "../utils/type_info.hpp" // TODO: berk
#include "../utils/uuid.cpp"      // TODO: berk

enum Status
{
    Unloaded, // All m_components are unloaded
    Loaded,   // All m_components are loaded (some might still be loading (dynamic add))
    Activated // Has been turned on in the world, registered with all m_systems
};

class Component;
class EntitySystem;

class EntityInternalStateAction
{
  public:
    enum Type
    {
        DestroyComponent,
        AddComponent,
        CreateSystem,
        DestroySystem
    };

    Type m_type;     // Type of action add/destroy components or systems
    void* m_ptr;     // Pointer to the designed Component or system TypeInfo
    core::UUID m_ID; // Optional: ID of the spatial parent component
};

// TODO: Move somewhere where this makes sense
class Command
{
  public:
    void Execute(Entity* pEntity)
    {
        // Read the deferredActions, probably store them somewhere
        // Asynchronously execute them

        // When should the updates occur ? Probably sometime inbetween frames
        // TODO: Where does the context come from ?

        // Example for one:
        ObjectModel::LoadingContext context; // TODO
        for (auto action : pEntity->m_deferredActions)
        {
            switch (action.m_type)
            {
            case EntityInternalStateAction::Type::AddComponent:
                auto pParentComponent = pEntity->GetSpatialComponent(action.m_ID);
                pEntity->AddComponentDeferred(context, (Component*) action.m_ptr, pParentComponent);
                break;
            case EntityInternalStateAction::Type::CreateSystem:
                pEntity->CreateSystemDeferred(context, (TypeInfo<EntitySystem>*) action.m_ptr);
                break;
            case EntityInternalStateAction::Type::DestroyComponent:
                pEntity->DestroyComponentDeferred(context, (Component*) action.m_ptr);
                break;
            case EntityInternalStateAction::Type::DestroySystem:
                pEntity->DestroySystemDeferred(context, (TypeInfo<EntitySystem>*) action.m_ptr);
                break;
            default:
                throw std::runtime_error("Unsupported operation");
            }
            // Remove the action from the list if it has been executed successfully
        }
    }
};

class Entity
{
    friend Command;

  private:
    core::UUID m_ID;
    Status m_status = Status::Unloaded;

    std::vector<Component*> m_components;
    std::vector<EntitySystem*> m_systems;

    std::array<std::vector<EntitySystem*>, UpdateStage::NumStages> m_systemUpdateLists;

    // Spacial attributes
    SpatialComponent* m_pRootSpatialComponent = nullptr;
    Entity* m_pParentSpatialEntity = nullptr; // A spatial entity may request to be attached to another spatial entity
    std::vector<Entity*> m_attachedEntities;  // Children
    core::UUID m_parentAttachmentSocketID;    // TODO: ?
    bool m_isAttachedToParent = false;

    // TODO:
    static Command EntityStateUpdatedEvent;
    std::vector<EntityInternalStateAction> m_deferredActions;

    // Constructor is private to prevent extending this class
    Entity(){};

    SpatialComponent* GetSpatialComponent(const core::UUID& spatialComponentID)
    {
        if (!spatialComponentID.IsValid())
        {
            return nullptr;
        }

        // assert(pSpatialComponent != nullptr);

        auto componentIt = std::find(m_components.begin(), m_components.end(), [spatialComponentID](Component* comp) { comp->GetID() == spatialComponentID; });
        assert(componentIt != m_components.end());

        auto pSpatialComponent = dynamic_cast<SpatialComponent*>(m_components[componentIt - m_components.begin()]);
        assert(pSpatialComponent != nullptr);
        return pSpatialComponent;
    }

  public:
    inline bool Entity::IsLoaded() const { return m_status == Status::Loaded; }
    inline bool Entity::IsUnloaded() const { return m_status == Status::Unloaded; }
    inline bool Entity::IsActivated() const { return m_status == Status::Activated; }
    inline bool Entity::IsSpatialEntity() const { return m_pRootSpatialComponent != nullptr; }
    inline core::UUID GetID() const { return m_ID; };

    void LoadComponents(const ObjectModel::LoadingContext& loadingContext);
    void UnloadComponents(const ObjectModel::LoadingContext& loadingContext);

    bool UpdateLoadingAndEntityState(const ObjectModel::LoadingContext& loadingContext);

    /// @brief Triggers registration with the systems (local and global)
    void Activate(const ObjectModel::LoadingContext& loadingContext);
    void Deactivate(const ObjectModel::LoadingContext& loadingContext);

    void GenerateSystemUpdateList();
    inline bool RequiresUpdate(UpdateStage stage) const { return !m_systemUpdateLists[(uint8_t) stage].empty(); }

    // -------------------------------------------------
    // Systems
    // -------------------------------------------------
    template <typename T>
    inline void CreateSystem()
    {
        static_assert(std::is_base_of_v<EntitySystem, T>, "Invalid system type");
        // TODO: assert that this entity doesn't already have a system of this type

        if (IsUnloaded())
        {
            CreateSystemImmediate(T::GetStaticTypeInfo().get());
        }
        else
        {
            auto& action = m_defferedAction.emplace_back(EntityInternalStateAction());
            action.m_type = EntityInternalStateAction::Type::CreateSystem;
            action.m_ptr = T::StaticTypeInfo;

            EntityStateUpdatedEvent.Execute(this);
        }
    }

    template <typename T>
    inline void DestroySystem()
    {
        static_assert(std::is_base_of_v<EntitySystem, T>);
        assert(std::find(m_systems, T::GetTypeInfo()->m_ID) != m_systems.end());

        if (IsUnloaded())
        {
            DestroySystemImmediate(T::GetTypeInfo()); // TODO: static type info...
        }
        else
        {
            auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
            action.m_type = EntityInternalStateAction::Type::DestroySystem;
            action.m_ptr = T::GetTypeInfo();

            EntityStateUpdateEvent.Execute(this);
        }
    }

    void CreateSystemImmediate(const TypeInfo<EntitySystem>* pSystemTypeInfo);
    void CreateSystemDeferred(const ObjectModel::LoadingContext& loadingContext, const TypeInfo<EntitySystem>* pSystemTypeInfo);
    void DestroySystemImmediate(const TypeInfo<EntitySystem>* pSystemTypeInfo);
    void DestroySystemDeferred(const ObjectModel::LoadingContext& loadingContext, const TypeInfo<EntitySystem>* pSystemTypeInfo);

    void UpdateSystems(const ObjectModel::UpdateContext& context);

    void DestroyComponent(const core::UUID& componentID);
    void DestroyComponentImmediate(Component* pComponent);
    void DestroyComponentDeferred(const ObjectModel::LoadingContext& loadingContext, Component* pComponent);

    void AddComponent(Component* pComponent, const core::UUID& parentSpatialComponentID = core::UUID::InvalidID);
    void AddComponentImmediate(Component* pComponent, SpatialComponent* pParentComponent);
    void AddComponentDeferred(const ObjectModel::LoadingContext& loadingContext, Component* pComponent, SpatialComponent* pParentComponent);

    SpatialComponent* FindSocketAttachmentComponent(SpatialComponent* pComponentToSearch, const core::UUID& socketID) const;

    inline SpatialComponent* FindSocketAttachmentComponent(const core::UUID& socketID) const
    {
        // TODO: that's freestyle
        assert(IsSpatialEntity());
        return FindSocketAttachmentComponent(m_pRootSpatialComponent, socketID);
    };

    void AttachToParent();
    void DetachFromParent();
    void RefreshEntityAttachments();
};