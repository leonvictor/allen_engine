#pragma once

#include "object_model.cpp"
#include "spatial_component.cpp"

#include <assert.h>
#include <stdexcept>
#include <vector>

#include "../utils/uuid.cpp" // TODO: berk

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

    Type m_type;
    void* m_ptr;
    UUID m_ID;
};

class Entity
{
  private:
    UUID m_ID;
    Status m_status = Status::Unloaded;

    std::vector<Component*> m_components;
    std::vector<EntitySystem*> m_systems;

    std::vector<EntitySystem*> m_systemUpdateLists[UpdateStage::NumStages];

    SpatialComponent* m_pRootSpatialComponent = nullptr;

    Entity* m_pParentSpatialEntity = nullptr; // A spatial entity may request to be attached to another spatial entity
    std::vector<Entity*> m_attachedEntities;  // Children
    UUID m_parentAttachmentSocketID;          // TODO: ?
    bool m_isAttachedToParent = false;

    std::vector<EntityInternalStateAction> m_deferredActions;

    // Constructor is private to prevent extending this class
    Entity(){};

  public:
    inline bool Entity::IsLoaded() const { return m_status == Status::Loaded; }
    inline bool Entity::IsUnloaded() const { return m_status == Status::Unloaded; }
    inline bool Entity::IsActivated() const { return m_status == Status::Activated; }
    inline bool Entity::IsSpatialEntity() const { return m_pRootSpatialComponent != nullptr; }
    inline UUID GetID() const { return m_ID; };

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
        static_assert(std::is_base_of<EntitySystem, T>::value, "Invalid system type");
        // TODO: assert that this entity doesn't already have a system of this type

        if (IsUnloaded())
        {
            CreateSystemImmediate(T::StaticTypeInfo);
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
        static_assert(std::is_base_of<EntitySystem, T>::value);
        // TODO: assert that m_systems contains an entity of this type
        // help: krg uses T::StaticTypeInfo->m_ID ...

        if (IsUnloaded())
        {
            DestroySystemImmediate(T::StaticTypeInfo);
        }
        else
        {
            auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
            action.m_type = EntityInternalStateAction::Type::DestroySystem;
            action.m_ptr = T::StaticTypeInfo;

            EntityStateUpdateEvent.Exectute(this);
        }
    }
    void CreateSystemImmediate(const TypeSystem::TypeInfo* pSystemTypeInfo);
    void CreateSystemDeferred(const ObjectModel::LoadingContext& loadingContext, const TypeSystem::TypeInfo* pSystemTypeInfo);
    void DestroySystemImmediate(const TypeSystem::TypeInfo* pSystemTypeInfo);
    void DestroySystemDeferred(const ObjectModel::LoadingContext& loadingContext, const TypeSystem::TypeInfo* pSystemTypeInfo);

    void UpdateSystems(ObjectModel::UpdateContext const& context);

    void DestroyComponent(const UUID& componentID);
    void DestroyComponentImmediate(Component* pComponent);
    void DestroyComponentDeferred(const ObjectModel::LoadingContext& loadingContext, Component* pComponent);

    void AddComponent(Component* pComponent, const UUID& parentSpatialComponentID = UUID::InvalidID);
    void AddComponentImmediate(Component* pComponent, SpatialComponent* pParentComponent);
    void AddComponentDeferred(const ObjectModel::LoadingContext& loadingContext, Component* pComponent, SpatialComponent* pParentComponent);

    SpatialComponent* FindSocketAttachmentComponent(SpatialComponent* pComponentToSearch, const UUID& socketID) const;
    inline SpatialComponent* FindSocketAttachmentComponent(UUID socketID) const
    {
        // TODO: that's freestyle
        assert(IsSpatialEntity());
        return FindSocketAttachmentComponent(m_pRootSpatialComponent, socketID);
    };

    void AttachToParent();
    void DetachFromParent();
    void RefreshEntityAttachments();
};