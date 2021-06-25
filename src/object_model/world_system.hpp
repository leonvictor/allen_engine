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

  protected:
    virtual const UpdatePriorities& GetUpdatePriorities() = 0;

    // Called when the system is registered with the world
    // virtual void Initialize(const SystemRegistry& systemRegistry) {};

    /// @brief Called when the system is removed from the world.
    virtual void Shutdown() = 0;

    /// @brief System update
    virtual void Update(const ObjectModel::UpdateContext& context) = 0;

    /// @brief Called whenever a new component is activated (added to the world).
    virtual void RegisterComponent(const Entity* pEntity, IComponent* pComponent) = 0;

    /// @brief Called immediately before a component is deactivated.
    virtual void UnregisterComponent(const Entity* pEntity, IComponent* pComponent) = 0;

  private:
    enum class Status
    {
        Disabled,
        Enabled,
    };

    Status m_status = Status::Disabled;
};