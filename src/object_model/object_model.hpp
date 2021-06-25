#pragma once

#include "../update_stages.hpp"

class Entity;
class IComponent;

namespace ObjectModel
{
class LoadingContext
{
  public:
    // TODO !!!!
    void m_unregisterWithGlobalSystems(Entity* pEntity, IComponent* pComponent) const {}
    void m_registerWithGlobalSystems(Entity* pEntity, IComponent* pComponent) const {}
    void m_unregisterEntityUpdate(Entity* pEntity) const {}
    void m_registerEntityUpdate(Entity* pEntity) const {}
};

/// @brief Data structure containing info on the current context.
/// Will be propagated through the object model, entities and systems will act depending on it.
/// @note This is why it's a class, we don't want to update the current stage mid-flight for example.
/// @todo I think it'd be better to just friend the class creating contexts (the main loop ?)
class UpdateContext
{
  private:
    UpdateStage m_updateStage;

  public:
    UpdateContext(UpdateStage stage) : m_updateStage(stage) {}

    UpdateStage GetUpdateStage() const
    {
        return m_updateStage;
    }
};
} // namespace ObjectModel