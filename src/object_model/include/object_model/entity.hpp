#pragma once

#include "command.hpp"
#include "entity_system.hpp"
#include "loading_context.hpp"
#include "object_model.hpp"

#include <assert.h>
#include <stdexcept>
#include <string>
#include <vector>

#include <utils/type_info.hpp>
#include <utils/uuid.hpp>

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
        DestroySystem
    };

    Type m_type;     // Type of action add/destroy components or systems
    void* m_ptr;     // Pointer to the designed IComponent or system TypeInfo
    core::UUID m_ID; // Optional: ID of the spatial parent component
};

/// @brief An Entity represents a single element in a scene/world. They're actual objects,
/// and cannot be derived from. They can hold various components and systems.
/// Entities can be organized in hierarchies.
class Entity
{
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
    const core::UUID m_ID;
    std::string m_name;
    Status m_status = Status::Unloaded;

    std::vector<IComponent*> m_components;
    std::vector<std::shared_ptr<IEntitySystem>> m_systems;

    // TODO: Replace by dedicated struct
    std::array<std::vector<std::shared_ptr<IEntitySystem>>, UpdateStage::NumStages> m_systemUpdateLists;

    // Spatial attributes
    SpatialComponent* m_pRootSpatialComponent = nullptr;
    Entity* m_pParentSpatialEntity = nullptr; // A spatial entity may request to be attached to another spatial entity
    std::vector<Entity*> m_attachedEntities;  // Children spatial entities
    core::UUID m_parentAttachmentSocketID;    // TODO: ?
    bool m_isAttachedToParent = false;

    // TODO:
    static Command EntityStateUpdatedEvent;
    std::vector<EntityInternalStateAction> m_deferredActions;

    // Constructor is private to prevent extending this class
    Entity(){};

    SpatialComponent* GetSpatialComponent(const core::UUID& spatialComponentID);

    /// @brief Create a new system and add it to this Entity.
    /// An Entity can only have one system of a given type (subtypes included).
    void CreateSystemImmediate(TypeInfo<IEntitySystem>* pSystemTypeInfo);

    /// @brief Same as CreateSystemImmediate, but the world system is notified that it should reload the Entity.
    void CreateSystemDeferred(const ObjectModel::LoadingContext& loadingContext, TypeInfo<IEntitySystem>* pSystemTypeInfo);
    void DestroySystemImmediate(const TypeInfo<IEntitySystem>* pSystemTypeInfo);
    void DestroySystemDeferred(const ObjectModel::LoadingContext& loadingContext, const TypeInfo<IEntitySystem>* pSystemTypeInfo);

    void DestroyComponentImmediate(IComponent* pComponent);
    void DestroyComponentDeferred(const ObjectModel::LoadingContext& loadingContext, IComponent* pComponent);
    void AddComponentImmediate(IComponent* pComponent, SpatialComponent* pParentComponent);
    void AddComponentDeferred(const ObjectModel::LoadingContext& loadingContext, IComponent* pComponent, SpatialComponent* pParentComponent);

    void RefreshEntityAttachments();

  public:
    inline bool IsLoaded() const { return m_status == Status::Loaded; }
    inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
    inline bool IsActivated() const { return m_status == Status::Activated; }
    inline bool IsSpatialEntity() const { return m_pRootSpatialComponent != nullptr; }
    inline const core::UUID& GetID() const { return m_ID; };
    std::string GetName() const { return m_name; }

    void LoadComponents(const ObjectModel::LoadingContext& loadingContext);
    void UnloadComponents(const ObjectModel::LoadingContext& loadingContext);

    /// @brief TODO
    /// @return Whether the loading is finished.
    bool UpdateLoadingAndEntityState(const ObjectModel::LoadingContext& loadingContext);

    /// @brief Triggers registration with the systems (local and global)
    void Activate(const ObjectModel::LoadingContext& loadingContext);

    /// @brief Unregister from local and global systems. Will also detach from the parent entity if applicable.
    /// After deactivation an entity will be in the Loaded state.
    void Deactivate(const ObjectModel::LoadingContext& loadingContext);

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
            CreateSystemImmediate(IEntitySystem::GetStaticTypeInfo<T>());
        }
        else
        {
            // Delegate the action to whoever is in charge
            auto& action = m_deferredActions.emplace_back(EntityInternalStateAction());
            action.m_type = EntityInternalStateAction::Type::CreateSystem;
            action.m_ptr = IEntitySystem::GetStaticTypeInfo<T>();

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

            EntityStateUpdatedEvent.Execute(this);
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
    void AddComponent(IComponent* pComponent, const core::UUID& parentSpatialComponentID = core::UUID::InvalidID);

    /// @brief Create a component of type T and add it to this entity.
    /// @param args: Arguments forwarded to the component constructor.
    /// @return A pointer to the constructed component
    /// @todo Experiment with other memory handling methods
    /// @todo Support parent spatial component in an elegant way
    template <typename T, class... Args>
    T* AddComponent(Args... args)
    {
        static_assert(std::is_base_of_v<IComponent, T>, "Invalid component type");

        T* pComponent = new T(args...);
        AddComponent(pComponent);
        return pComponent;
    }

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

    /// Factory methods
    // TODO: Is that how we want to generate entities ?
    static Entity* Create(std::string name);

    inline bool operator==(const Entity& other) const { return other.GetID() == GetID(); }
    inline bool operator!=(const Entity& other) const { return !operator==(other); }
    friend bool operator<(const Entity& l, const Entity& r) { return l.m_ID < r.m_ID; }
};