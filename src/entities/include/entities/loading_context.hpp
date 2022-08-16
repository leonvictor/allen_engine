#pragma once

#include <functional>
#include <vector>

namespace aln
{
class TaskService;
}

namespace aln::entities
{
class Entity;
class IComponent;

struct LoadingContext
{
    TaskService* m_pTaskService = nullptr;
    AssetService* m_pAssetService = nullptr;

    /// @brief Callback to notify the world entity that a component/entity pair should be unregistered from the world systems.
    std::function<void(Entity*, IComponent*)> m_unregisterWithWorldSystems;
    /// @brief Callback to notify the world entity that a new component/entity pair should be registered with the world systems.
    std::function<void(Entity*, IComponent*)> m_registerWithWorldSystems;
    /// @brief Callback to register an entity update requirement list to the world.
    std::function<void(Entity*)> m_registerEntityUpdate;
    /// @brief Callback to unregister an entity update requirement list to the world.
    std::function<void(Entity*)> m_unregisterEntityUpdate;

    LoadingContext() = default;
    LoadingContext(TaskService* pTaskService)
        : m_pTaskService(pTaskService) {}

    bool IsInitialized() { return m_pTaskService != nullptr; }
};
} // namespace aln::entities