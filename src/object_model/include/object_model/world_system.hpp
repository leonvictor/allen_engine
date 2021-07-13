#pragma once

// https://www.youtube.com/watch?v=jjEsB611kxs @1:49:00

namespace ObjectModel
{
class UpdateContext;
}
class UpdatePriorities;
class Entity;
class IComponent;

/// @brief Interface for world systems. World systems are the ones that do not depend on separate entities.
/// @note Local systems always runs before global
/// @note World systems are singletons (only of of each type)
class IWorldSystem
{
    friend class WorldEntity;

  private:
    enum class Status
    {
        Uninitialized,
        Initialized,
        Enabled,
    };

    Status m_status = Status::Uninitialized;

    // Wrapper method used by the world entity.
    // Used to hide the status from user code.
    void InitializeSystem()
    {
        assert(m_status == Status::Uninitialized);
        Initialize();
        m_status = Status::Initialized;
    }

    void ShutdownSystem()
    {
        assert(m_status == Status::Initialized);
        Shutdown();
        m_status = Status::Uninitialized;
    }

  protected:
    virtual const UpdatePriorities& GetUpdatePriorities() = 0;

    // TODO: Called when the system is registered with the world
    // virtual void Initialize(const SystemRegistry& systemRegistry) {};
    virtual void Initialize() = 0;

    /// @brief Called when the system is removed from the world.
    virtual void Shutdown() = 0;

    /// @brief System update
    virtual void Update(const ObjectModel::UpdateContext& context) = 0;

    /// @brief Called whenever a new component is activated (added to the world).
    virtual void RegisterComponent(const Entity* pEntity, IComponent* pComponent) = 0;

    /// @brief Called immediately before a component is deactivated.
    virtual void UnregisterComponent(const Entity* pEntity, IComponent* pComponent) = 0;

    void Enable()
    {
        assert(m_status == Status::Initialized);
        m_status = Status::Enabled;
    }

    void Disable()
    {
        assert(m_status == Status::Enabled);
        m_status = Status::Initialized;
    }
};