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
class IEntitySystem;

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
                pEntity->CreateSystemDeferred(context, (TypeInfo<IEntitySystem>*) action.m_ptr);
                break;
            case EntityInternalStateAction::Type::DestroyComponent:
                pEntity->DestroyComponentDeferred(context, (Component*) action.m_ptr);
                break;
            case EntityInternalStateAction::Type::DestroySystem:
                pEntity->DestroySystemDeferred(context, (TypeInfo<IEntitySystem>*) action.m_ptr);
                break;
            default:
                throw std::runtime_error("Unsupported operation");
            }
            // Remove the action from the list if it has been executed successfully
        }
    }
};

/// @brief An Entity represents a single element in a scene/world.
/// Entities can be organized in hierarchies.
class Entity
{
    friend Command;

  private:
    core::UUID m_ID;
    Status m_status = Status::Unloaded;

    std::vector<Component*> m_components;
    std::vector<IEntitySystem*> m_systems;

    std::array<std::vector<IEntitySystem*>, UpdateStage::NumStages> m_systemUpdateLists;

    // Spatial attributes
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

        auto componentIt = std::find(m_components.begin(), m_components.end(), [spatialComponentID](Component* comp)
                                     { comp->GetID() == spatialComponentID; });
        assert(componentIt != m_components.end());

        auto pSpatialComponent = dynamic_cast<SpatialComponent*>(m_components[componentIt - m_components.begin()]);
        assert(pSpatialComponent != nullptr);
        return pSpatialComponent;
    }

    /// @brief Create a new system and add it to this Entity.
    /// An Entity can only have one system of a given type (subtypes included).
    void CreateSystemImmediate(const TypeInfo<IEntitySystem>* pSystemTypeInfo);

    /// @brief Same as CreateSystemImmediate, but the world system is notified that it should reload the Entity.
    void CreateSystemDeferred(const ObjectModel::LoadingContext& loadingContext, const TypeInfo<IEntitySystem>* pSystemTypeInfo);
    void DestroySystemImmediate(const TypeInfo<IEntitySystem>* pSystemTypeInfo);
    void DestroySystemDeferred(const ObjectModel::LoadingContext& loadingContext, const TypeInfo<IEntitySystem>* pSystemTypeInfo);

    void DestroyComponentImmediate(Component* pComponent);
    void DestroyComponentDeferred(const ObjectModel::LoadingContext& loadingContext, Component* pComponent);
    void AddComponentImmediate(Component* pComponent, SpatialComponent* pParentComponent);
    void AddComponentDeferred(const ObjectModel::LoadingContext& loadingContext, Component* pComponent, SpatialComponent* pParentComponent);

    void RefreshEntityAttachments();

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

    /// @brief Create a System of type T and add it to this Entity.
    template <typename T>
    inline void CreateSystem()
    {
        static_assert(std::is_base_of_v<IEntitySystem, T>, "Invalid system type");
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

    /// @brief Destroy the system of type T of this Entity. The destruction is effective
    /// immediately if the entity is unloaded, otherwise it will be effective in the next frame.
    template <typename T>
    inline void DestroySystem()
    {
        static_assert(std::is_base_of_v<IEntitySystem, T>);
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

    /// @brief Update all systems attached to this entity.
    void UpdateSystems(const ObjectModel::UpdateContext& context);

    /// @brief Destroy a component from this entity.
    /// @param componentID: UUID of the component to destroy.
    void DestroyComponent(const core::UUID& componentID);

    /// @brief Add a component to this entity.
    /// @param pComponent: Component to add.
    /// @param parentSpatialComponentID: Only when adding a spatial component. UUID of the spatial component to attach to.
    void AddComponent(Component* pComponent, const core::UUID& parentSpatialComponentID = core::UUID::InvalidID);

    SpatialComponent* FindSocketAttachmentComponent(SpatialComponent* pComponentToSearch, const core::UUID& socketID) const;

    inline SpatialComponent* FindSocketAttachmentComponent(const core::UUID& socketID) const
    {
        // TODO: that's freestyle
        assert(IsSpatialEntity());
        return FindSocketAttachmentComponent(m_pRootSpatialComponent, socketID);
    };

    /// @brief Attach to the parent entity.
    void AttachToParent();

    /// @brief Detach from the parent entity.
    void DetachFromParent();
};