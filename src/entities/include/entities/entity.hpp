#pragma once

#include "entity_system.hpp"
#include "loading_context.hpp"
#include "update_context.hpp"

#include <common/containers/vector.hpp>
#include <common/event.hpp>
#include <common/uuid.hpp>
#include <reflection/type_info.hpp>

#include <Tracy.hpp>

#include <assert.h>
#include <set>
#include <stdexcept>
#include <string>

namespace aln
{

// fwd
class IComponent;
class SpatialComponent;
class TypeRegistryService;

class EntityInternalStateAction
{
  public:
    /// @brief Type of Actions
    enum Type
    {
        DestroyComponent,
        AddComponent,
        CreateSystem,
        DestroySystem,
    };

    Type m_type;       // Type of action
    const void* m_ptr; // Pointer to the designed IComponent or system TypeInfo
    UUID m_ID;         // Optional: ID of the spatial parent component
};

/// @brief An Entity represents a single element in a scene/world. They're actual objects,
/// and cannot be derived from. They can hold various components and systems.
/// Entities can be organized in hierarchies.
class Entity
{
    friend class EntityMap;
    friend class EntityDescriptor;
    friend class EntityMapDescriptor;

    enum class Status
    {
        Unloaded, // All components are unloaded
        Loaded,   // All components are loaded (some might still be loading (dynamic add))
        Activated // Has been turned on in the world, registered with all systems
    };

  private:
    const UUID m_ID = UUID::Generate();
    std::string m_name;
    Status m_status = Status::Unloaded;

    Vector<IComponent*> m_components;
    Vector<IEntitySystem*> m_systems;

    // TODO: Replace by dedicated struct
    std::array<Vector<IEntitySystem*>, UpdateStage::NumStages> m_systemUpdateLists;

    // Spatial attributes
    SpatialComponent* m_pRootSpatialComponent = nullptr;
    Entity* m_pParentSpatialEntity = nullptr; // A spatial entity may request to be attached to another spatial entity
    Vector<Entity*> m_attachedEntities;       // Children spatial entities
    aln::UUID m_parentAttachmentSocketID;     // TODO: ?
    bool m_isAttachedToParent = false;

    static Event<Entity*> EntityStateUpdatedEvent;
    Vector<EntityInternalStateAction> m_deferredActions;

    SpatialComponent* GetSpatialComponent(const UUID& spatialComponentID);

    /// @brief Create a new system and add it to this Entity.
    /// An Entity can only have one system of a given type (subtypes included).
    void CreateSystemImmediate(const aln::reflect::TypeInfo* pSystemTypeInfo);

    void CreateSystemDeferred(const LoadingContext& loadingContext, const aln::reflect::TypeInfo* pSystemTypeInfo);
    void DestroySystemImmediate(const aln::reflect::TypeInfo* pSystemTypeInfo);
    void DestroySystemDeferred(const LoadingContext& loadingContext, const aln::reflect::TypeInfo* pSystemTypeInfo);

    void DestroyComponentImmediate(IComponent* pComponent);
    void DestroyComponentDeferred(const LoadingContext& loadingContext, IComponent* pComponent);
    void AddComponentImmediate(IComponent* pComponent, SpatialComponent* pParentComponent);
    void AddComponentDeferred(const LoadingContext& loadingContext, IComponent* pComponent, SpatialComponent* pParentComponent);

    void RegisterComponentWithEntitySystems(IComponent* pComponent);
    void UnregisterComponentWithEntitySystems(IComponent* pComponent);

    /// @brief Attach to the parent entity.
    void AttachToParent();
    /// @brief Detach from the parent entity.
    void DetachFromParent();

    void RemoveChild(Entity* pEntity);
    void AddChild(Entity* pEntity);
    void RefreshEntityAttachments();

    void HandleDeferredActions(const LoadingContext& loadingContext);

  public:
    /// @todo: Constructor is private to prevent extending this class
    Entity(){};
    ~Entity();

    const UUID GetID() const { return m_ID; };
    std::string& GetName() { return m_name; }

    /// @brief Whether this entity is loaded (some components might still be loading in case of dynamic add)
    bool IsLoaded() const { return m_status == Status::Loaded || m_status == Status::Activated; }
    bool IsUnloaded() const { return m_status == Status::Unloaded; }
    bool IsActivated() const { return m_status == Status::Activated; }

    bool IsSpatialEntity() const { return m_pRootSpatialComponent != nullptr; }
    bool HasParentEntity() const { return m_pParentSpatialEntity != nullptr; }
    bool HasChildrenEntities() const { return !m_attachedEntities.empty(); }
    const Vector<Entity*> GetChildren() const { return m_attachedEntities; }

    /// @todo Consider implications of this being public ?
    SpatialComponent* GetRootSpatialComponent() { return m_pRootSpatialComponent; }

    void LoadComponents(const LoadingContext& loadingContext);
    void UnloadComponents(const LoadingContext& loadingContext);

    /// @brief Check the loading status of all components and updates the entity status if necessary.
    /// @return Whether the loading is finished
    /// @todo This is synchrone for now
    bool UpdateLoadingAndEntityState(const LoadingContext& loadingContext);

    /// @brief Triggers registration with the systems (local and global)
    void Activate(const LoadingContext& loadingContext);

    /// @brief Unregister from local and global systems. Will also detach from the parent entity if applicable.
    /// After deactivation an entity will be in the Loaded state.
    void Deactivate(const LoadingContext& loadingContext);

    /// @brief Generate the system update list by gathering each system's requirements.
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
        CreateSystem(T::GetStaticTypeInfo());
    }

    void CreateSystem(const aln::reflect::TypeInfo* pTypeInfo);

    /// @brief Destroy the system of type T of this Entity. The destruction is effective
    /// immediately if the entity is unloaded, otherwise it will be effective in the next frame.
    template <typename T>
    inline void DestroySystem()
    {
        static_assert(std::is_base_of_v<IEntitySystem, T>);
        DestroySystem(T::GetStaticTypeInfo());
    }

    /// @brief Destroy a system.
    /// @todo used by the editor, should this be public ?
    void DestroySystem(const aln::reflect::TypeInfo* pTypeInfo);

    /// @brief Update all systems attached to this entity.
    void UpdateSystems(const UpdateContext& context) const;

    const Vector<IEntitySystem*>& GetSystems() { return m_systems; }

    // -------------------------------------------------
    // Components
    // -------------------------------------------------

    /// @brief Destroy a component from this entity
    /// @param componentID: UUID of the component to destroy.
    void DestroyComponent(const UUID& componentID);

    /// @brief Add a component to this entity, taking ownership of it
    /// @param pComponent: Component to add.
    /// @param parentSpatialComponentID: Only when adding a spatial component. UUID of the spatial component to attach to.
    void AddComponent(IComponent* pComponent, const UUID& parentSpatialComponentID = UUID::InvalidID);

    const Vector<IComponent*>& GetComponents() { return m_components; }

    SpatialComponent* FindSocketAttachmentComponent(SpatialComponent* pComponentToSearch, const UUID& socketID) const;

    inline SpatialComponent* FindSocketAttachmentComponent(const UUID& socketID) const
    {
        // TODO: that's freestyle
        assert(IsSpatialEntity());
        return FindSocketAttachmentComponent(m_pRootSpatialComponent, socketID);
    };

    /// @brief Add a parent entity.
    void SetParentEntity(Entity* pEntity);

    // -------- Editing
    // TODO: Disable in prod

    void StartComponentEditing(const LoadingContext& loadingContext, IComponent* pComponent);
    void EndComponentEditing(const LoadingContext& loadingContext);

    inline bool operator==(const Entity& other) const { return other.GetID() == GetID(); }
    inline bool operator!=(const Entity& other) const { return !operator==(other); }
    friend bool operator<(const Entity& l, const Entity& r) { return l.m_ID < r.m_ID; }
};
} // namespace aln