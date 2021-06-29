#pragma once

#include "../utils/uuid.hpp"
#include <future>

// TODO: Wrap status changes method to automatically run assertions in order to avoid having to rely on user classes adding them systematically
/// @brief Data storage attached to Entities.
/// @note https://www.youtube.com/watch?v=jjEsB611kxs : 1:34:50
class IComponent
{
    friend class Entity;
    // TODO: Serialization
    // TODO: is this a singleton ?

  private:
    /// @brief Each component can ben in one of the states
    enum class Status
    {
        Unloaded,      // Constructed and all properties set
        Loading,       // Resource loading in progress
        Loaded,        // All resources loaded successfully
        LoadingFailed, // Some resources failed to load
        Initialized    // Ready to use
    };

    core::UUID m_ID;
    Status m_status = Status::Unloaded;
    bool m_isSingleton = false;                    // Whether you can have multiple components of this type per entity
    core::UUID m_entityID = core::UUID::InvalidID; // Entity this component is attached to.

    std::future<bool> m_loadingTask;

    // Private state management methods. Use verbose naming to differentiate with the inner virtual ones.
    bool LoadComponent();
    void InitializeComponent();
    void ShutdownComponent();
    void UnloadComponent();

    bool LoadComponentAsync();

  protected:
    /// @brief Allocate internal transient data.
    /// @todo: Occurs automatically once loading completes sucessfully.
    virtual void Initialize() = 0;

    /// @brief Deallocate internal transient data.
    virtual void Shutdown() = 0;

    /// @brief Load all ressources.
    /// @return Whether loading was successful.
    virtual bool Load() = 0;

    /// @brief Unload all ressources.
    virtual void Unload() = 0;

  public:
    inline bool IsInitialized() const { return m_status == Status::Initialized; }
    inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
    inline bool IsLoading() const { return m_status == Status::Loading; }
    inline bool IsLoaded() const { return m_status == Status::Loaded; }

    // TODO: Should be a copy to prevent modifying. Requires copy constructor in core::UUID
    const core::UUID& GetID() const { return m_ID; }

    bool operator==(const IComponent& other) const { return m_ID == other.GetID(); }
    bool operator!=(const IComponent& other) const { return !operator==(other); }
};