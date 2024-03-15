#pragma once

#include <common/uuid.hpp>
#include <future>
#include <reflection/reflected_type.hpp>

#include "loading_context.hpp"

namespace aln
{

/// @brief Data storage attached to Entities.
/// @note https://www.youtube.com/watch?v=jjEsB611kxs : 1:34:50
class IComponent : public reflect::IReflected
{
    friend class Entity;
    friend class WorldEntity;

  private:
    const UUID m_ID = UUID::Generate();
    UUID m_entityID = UUID::InvalidID; // Entity this component is attached to.
    bool m_isSingleton = false;          // Whether you can have multiple components of this type per entity

    bool m_registeredWithEntitySystems = false;
    bool m_registeredWithWorldSystems = false;

    // Private state management methods. Use verbose naming to differentiate with the inner virtual ones.
    void LoadComponent(const LoadingContext& loadingContext);
    void UnloadComponent(const LoadingContext& loadingContext);
    void InitializeComponent();
    void ShutdownComponent();

  protected:
    /// @brief Each component can ben in one of the states
    enum class Status
    {
        Unloaded,      // Constructed and all properties set
        Loading,       // Resource loading in progress
        Loaded,        // All resources loaded successfully
        LoadingFailed, // Some resources failed to load
        Initialized    // Ready to use
    };

    /// @brief Allocate internal transient data.
    /// Occurs automatically once loading completes sucessfully.
    virtual void Initialize() = 0;

    /// @brief Deallocate internal transient data.
    virtual void Shutdown() = 0;

    /// @brief Load all ressources.
    virtual void Load(const LoadingContext& loadingContext) = 0;

    /// @brief Unload all ressources.
    virtual void Unload(const LoadingContext& loadingContext) = 0;

    /// @brief Check the loading status of necessary resources and update the component status accordingly
    /// @todo For now each component needs to override this and manually handle its status.
    virtual bool UpdateLoadingStatus()
    {
        m_status = Status::Loaded;
        return true;
    };

    // @note: this was moved to protected because the status can be changed during UpdateLoadingStatus in derived classes.
    // @todo: Find a way to do this automatically
    Status m_status = Status::Unloaded;

  public:
    virtual ~IComponent() {}

    inline bool IsInitialized() const { return m_status == Status::Initialized; }
    inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
    inline bool IsLoading() const { return m_status == Status::Loading; }
    inline bool IsLoaded() const { return m_status == Status::Loaded; }
    inline bool HasFailedLoading() const { return m_status == Status::LoadingFailed; }

    inline bool IsRegisteredWithEntitySystems() const { return m_registeredWithEntitySystems; }
    inline bool IsRegisteredWithWorldSystems() const { return m_registeredWithWorldSystems; }

    const UUID& GetID() const { return m_ID; }
    const UUID& GetEntityID() const { return m_entityID; }

    bool operator==(const IComponent& other) const { return m_ID == other.GetID(); }
    bool operator!=(const IComponent& other) const { return !operator==(other); }
};
} // namespace aln