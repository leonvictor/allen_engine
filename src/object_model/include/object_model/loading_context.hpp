#pragma once

#include <functional>
#include <vector>

class Entity;
class IComponent;

namespace ObjectModel
{
struct LoadingContext
{
    /// @brief Callback to notify the world entity that a new component/entity pair should be registered with the world systems.
    std::function<void(Entity*, IComponent*)> m_unregisterWithWorldSystems;
    /// @brief Callback to notify the world entity that a new component/entity pair should be unregistered with the world systems.
    std::function<void(Entity*, IComponent*)> m_registerWithWorldSystems;
    /// @brief Callback to notify the world systems that the entity has been "turned on".
    std::function<void(Entity*)> m_registerEntityUpdate;
    /// @brief Callback to notify the world systems that an entity has been "turned off".
    std::function<void(Entity*)> m_unregisterEntityUpdate;
};
} // namespace ObjectModel