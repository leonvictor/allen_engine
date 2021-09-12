#pragma once

#include <functional>
#include <vector>

namespace aln::entities
{
class Entity;
class IComponent;

struct LoadingContext
{
    /// @brief Callback to notify the world entity that a component/entity pair should be unregistered from the world systems.
    std::function<void(Entity*, IComponent*)> m_unregisterWithWorldSystems;
    /// @brief Callback to notify the world entity that a new component/entity pair should be unregistered with the world systems.
    std::function<void(Entity*, IComponent*)> m_registerWithWorldSystems;
    /// @brief Callback to register an entity update requirement list to the world.
    std::function<void(Entity*)> m_registerEntityUpdate;
    /// @brief Callback to unregister an entity update requirement list to the world.
    std::function<void(Entity*)> m_unregisterEntityUpdate;
};
}