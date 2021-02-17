#include "../utils/uuid.cpp"
#include <assert.h>
#include <stdexcept>

/// Each component can ben in one of the states
// TODO: Allow bitwise operations
enum ComponentStatus
{
    Unloaded,      // Constructed and all properties set
    Loading,       // Resource loading in progress
    Loaded,        // All resources loaded successfully
    LoadingFailed, // Some resources failed to load
    Initialized    // TODO
};

// TODO: Wrap m_state changes method to automatically run assertions in order to avoid having to rely on user classes adding them systematically
/// @brief Data storage attached to Entities.
/// https://www.youtube.com/watch?v=jjEsB611kxs : 1:34:50
class Component
{
    // TODO: Serialization
    // TODO: is this a singleton ?

  protected:
    UUID m_ID;
    ComponentStatus m_state;
    bool m_isSingleton; // Can you have multiple components of this type per entity

  public:
    UUID m_entityID;

    Component() : m_state(ComponentStatus::Unloaded), m_isSingleton(false) {}

    virtual void Initialize()
    {
        // Initialization is not allowed to fail
        assert(m_state == ComponentStatus::Loaded);

        // TODO: unload all resources
        m_state = ComponentStatus::Initialized;
    };

    virtual void Shutdown()
    {
        assert(IsInitialized());

        // TODO: unload all resources
        m_state = ComponentStatus::Loaded;
    };

    virtual void Load()
    {
        assert(m_state == ComponentStatus::Unloaded);
        // TODO: load stuff
        // m_state = ComponentStatus::Loading;
        // TODO: set the correct m_state
        // m_state = ComponentStatus::LoadingFailed;
        m_state = ComponentStatus::Loaded;
    };

    virtual void Unload()
    {
        assert(m_state == ComponentStatus::Loaded || m_state == ComponentStatus::LoadingFailed || m_state == ComponentStatus::Loading);

        // TODO: unload all resources
        m_state = ComponentStatus::Unloaded;
    };

    bool IsInitialized() const { return m_state == ComponentStatus::Initialized; }
    bool IsLoaded() const { return m_state == ComponentStatus::Loaded; }
    bool IsUnloaded() const { return m_state == ComponentStatus::Unloaded; }

    UUID GetID() const { return m_ID; }

    bool operator==(const Component& other) const { return m_ID == other.m_ID; }
    bool operator!=(const Component& other) const { return !operator==(other); }
};