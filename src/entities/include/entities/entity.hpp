#pragma once

#include "command.hpp"
#include "entity_system.hpp"
#include "loading_context.hpp"
#include "object_model.hpp"

#include <assert.h>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <reflection/reflection.hpp>
#include <utils/uuid.hpp>

namespace aln::entities
{

// fwd
class IComponent;
class SpatialComponent;

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
        ParentChanged, // The entity's parent has changed
    };

    Type m_type;           // Type of action add/destroy components or systems
    const void* m_ptr;     // Pointer to the designed IComponent or system TypeInfo
    aln::utils::UUID m_ID; // Optional: ID of the spatial parent component
};

/// @brief An Entity represents a single element in a scene/world. They're actual objects,
/// and cannot be derived from. They can hold various components and systems.
/// Entities can be organized in hierarchies.
class Entity
{
    using UUID = aln::utils::UUID;

    friend Command;
    friend class EntityMap;
    friend class EntityCollection;
    friend class std::set<Entity>;

    enum class Status
    {
        Unloaded, // All components are unloaded
        Loaded,   // All components are loaded (some might still be loading (dynamic add))
        Activated // Has been turned on in the world, registered with all systems
    };

  private:
    const aln::utils::UUID m_ID;
    std::string m_name;
    Status m_status = Status::Unloaded;

    std::vector<IComponent*> m_components;
    std::vector<std::shared_ptr<IEntitySystem>> m_systems;

    // TODO: Replace by dedicated struct
    std::array<std::vector<std::shared_ptr<IEntitySystem>>, UpdateStage::NumStages> m_systemUpdateLists;

    // Spatial attributes
    SpatialComponent* m_pRootSpatialComponent = nullptr;
    Entity* m_pParentSpatialEntity = nullptr;    // A spatial entity may request to be attached to another spatial entity
    std::vector<Entity*> m_attachedEntities;     // Children spatial entities
    aln::utils::UUID m_parentAttachmentSocketID; // TODO: ?
    bool m_isAttachedToParent = false;

    // TODO:
    inline static Command EntityStateUpdatedEvent;
    std::vector<EntityInternalStateAction> m_deferredActions;

    // Constructor is private to prevent extending this class
    Entity(){};

    SpatialComponent* GetSpatialComponent(const UUID& spatialComponentID);

    /// @brief Create a new system and add it to this Entity.
    /// An Entity can only have one system of a given type (subtypes included).
    void CreateSystemImmediate(const aln::reflect::TypeDescriptor* pSystemTypeInfo);

    /// @brief Same as CreateSystemImmediate, but the world system is notified that it should reload the Entity.
    void CreateSystemDeferred(const LoadingContext& loadingContext, aln::reflect::TypeDescriptor* pSystemTypeInfo);
    void DestroySystemImmediate(const aln::reflect::TypeDescriptor* pSystemTypeInfo);
    void DestroySystemDeferred(const LoadingContext& loadingContext, const aln::reflect::TypeDescriptor* pSystemTypeInfo);

    void DestroyComponentImmediate(IComponent* pComponent);
    void DestroyComponentDeferred(const LoadingContext& loadingContext, IComponent* pComponent);
    void AddComponentImmediate(IComponent* pComponent, SpatialComponent* pParentComponent);
    void AddComponentDeferred(const LoadingContext& loadingContext, IComponent* pComponent, SpatialComponent* pParentComponent);

    /// @brief Attach to the parent entity.
    void AttachToParent();
    /// @brief Detach from the parent entity.
    void DetachFromParent();

    void RemoveChild(Entity* pEntity);
    void AddChild(Entity* pEntity);
    void RefreshEntityAttachments();

  public:
    bool IsLoaded() const { return m_status == Status::Loaded; }
    bool IsUnloaded() const { return m_status == Status::Unloaded; }
    bool IsActivated() const { return m_status == Status::Activated; }
    bool IsSpatialEntity() const { return m_pRootSpatialComponent != nullptr; }
    const UUID& GetID() const { return m_ID; };
    std::string GetName() const { return m_name; }
    bool HasParentEntity() const { return m_pParentSpatialEntity != nullptr; }
    bool HasChildrenEntities() const { return !m_attachedEntities.empty(); }
    const std::vector<Entity*> GetChildren() const
    {
        return m_attachedEntities;
    }

    /// @todo Consider implications of this being public ?
    SpatialComponent* GetRootSpatialComponent()
    {
        return m_pRootSpatialComponent;
    }

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

        if (IsUnloaded())
        {
            CreateSystemImmediate(T::GetStaticType());
        }
        else
        {
            // Delegate the action to whoever is in charge
            auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
            action.m_type = EntityInternalStateAction::Type::CreateSystem;
            action.m_ptr = T::GetStaticType();

            EntityStateUpdatedEvent.Execute(this);
        }
    }

    /// @brief Destroy the system of type T of this Entity. The destruction is effective
    /// immediately if the entity is unloaded, otherwise it will be effective in the next frame.
    template <typename T>
    inline void DestroySystem()
    {
        static_assert(std::is_base_of_v<IEntitySystem, T>);
        assert(std::find(m_systems, T::GetStaticType()->m_ID) != m_systems.end());

        if (IsUnloaded())
        {
            DestroySystemImmediate(T::GetStaticType()); // TODO: static type info...
        }
        else
        {
            auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
            action.m_type = EntityInternalStateAction::Type::DestroySystem;
            action.m_ptr = T::GetStaticType();

            EntityStateUpdatedEvent.Execute(this);
        }
    }

    /// @brief Update all systems attached to this entity.
    void UpdateSystems(const UpdateContext& context);

    // -------------------------------------------------
    // Components
    // -------------------------------------------------

    /// @brief Destroy a component from this entity.
    /// @param componentID: UUID of the component to destroy.
    void DestroyComponent(const UUID& componentID);

    /// @brief Add a component to this entity.
    /// @param pComponent: Component to add.
    /// @param parentSpatialComponentID: Only when adding a spatial component. UUID of the spatial component to attach to.
    void AddComponent(IComponent* pComponent, const UUID& parentSpatialComponentID = UUID::InvalidID());

    const std::vector<IComponent*>& GetComponents()
    {
        return m_components;
    }

    SpatialComponent* FindSocketAttachmentComponent(SpatialComponent* pComponentToSearch, const UUID& socketID) const;

    inline SpatialComponent* FindSocketAttachmentComponent(const UUID& socketID) const
    {
        // TODO: that's freestyle
        assert(IsSpatialEntity());
        return FindSocketAttachmentComponent(m_pRootSpatialComponent, socketID);
    };

    /// @brief Add a parent entity.
    void SetParentEntity(Entity* pEntity);

    /// Factory methods
    // TODO: Is that how we want to generate entities ?
    static Entity* Create(std::string name);

    inline bool operator==(const Entity& other) const { return other.GetID() == GetID(); }
    inline bool operator!=(const Entity& other) const { return !operator==(other); }
    friend bool operator<(const Entity& l, const Entity& r) { return l.m_ID < r.m_ID; }
};
} // namespace aln::entities