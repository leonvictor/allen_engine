#pragma once

// https://www.youtube.com/watch?v=jjEsB611kxs @1:49:00

namespace aln
{

class UpdateContext;
struct UpdatePriorities;
class Entity;
class IComponent;
class ServiceProvider;

/// @brief Interface for world systems. World systems are the ones that do not depend on separate entities.
/// @note Local systems always run before global ones
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

    // Wrapper methods used by the world entity.
    // Used to hide the status from user code.
    void InitializeSystem(const ServiceProvider& serviceProvider);
    void ShutdownSystem(const ServiceProvider& serviceProvider);

  protected:
    virtual const UpdatePriorities& GetUpdatePriorities() = 0;

    // TODO: Called when the system is registered with the world
    // virtual void Initialize(const SystemRegistry& systemRegistry) {};
    virtual void Initialize(const ServiceProvider& serviceProvider) = 0;

    /// @brief Called when the system is removed from the world.
    virtual void Shutdown(const ServiceProvider& serviceProvider) = 0;

    /// @brief System update
    virtual void Update(const UpdateContext& context) = 0;

    /// @brief Called whenever a new component is activated (added to the world).
    virtual void RegisterComponent(const Entity* pEntity, IComponent* pComponent) = 0;

    /// @brief Called immediately before a component is deactivated.
    virtual void UnregisterComponent(const Entity* pEntity, IComponent* pComponent) = 0;

    void Enable();

    void Disable();

  public:
    virtual ~IWorldSystem() {}
};
} // namespace aln