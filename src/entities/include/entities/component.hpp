#pragma once

#include <future>
#include <reflection/reflection.hpp>
#include <utils/uuid.hpp>

#include "component_creation_context.hpp"

namespace aln
{
// fwd
class ComponentFactory;

namespace entities
{

/// @brief Data storage attached to Entities.
/// @note https://www.youtube.com/watch?v=jjEsB611kxs : 1:34:50
class IComponent
{
    friend class Entity;
    friend class aln::ComponentFactory;
    // TODO: Serialization

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

    const aln::utils::UUID m_ID;
    Status m_status = Status::Unloaded;
    bool m_isSingleton = false;                                  // Whether you can have multiple components of this type per entity
    aln::utils::UUID m_entityID = aln::utils::UUID::InvalidID(); // Entity this component is attached to.

    std::future<bool> m_loadingTask;

    // Private state management methods. Use verbose naming to differentiate with the inner virtual ones.
    bool LoadComponent();
    bool LoadComponentAsync();
    void UnloadComponent();
    void InitializeComponent();
    void ShutdownComponent();

  protected:
    virtual void Construct(const ComponentCreationContext&) {}

    /// @brief Allocate internal transient data.
    /// Occurs automatically once loading completes sucessfully.
    virtual void Initialize() = 0;

    /// @brief Deallocate internal transient data.
    virtual void Shutdown() = 0;

    /// @brief Load all ressources.
    /// @return Whether loading was successful.
    virtual bool Load() = 0;

    /// @brief Unload all ressources.
    virtual void Unload() = 0;

  public:
    virtual ~IComponent() {}
    inline bool IsInitialized() const { return m_status == Status::Initialized; }
    inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
    inline bool IsLoading() const { return m_status == Status::Loading; }
    inline bool IsLoaded() const { return m_status == Status::Loaded; }

    const aln::utils::UUID& GetID() const { return m_ID; }

    bool operator==(const IComponent& other) const { return m_ID == other.GetID(); }
    bool operator!=(const IComponent& other) const { return !operator==(other); }

    ALN_REGISTER_TYPE()
};
} // namespace entities
} // namespace aln